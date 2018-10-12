#include "fileshvjournal.h"

#include <shv/chainpack/cponreader.h>
#include <shv/core/log.h>
#include <shv/core/string.h>

#include <fstream>

#include <dirent.h>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace utils {

static int uptimeSec()
{
	int uptime;
	if (std::ifstream("/proc/uptime", std::ios::in) >> uptime) {
		return uptime;
	}
	return 0;
}

const char * FileShvJournal::FILE_EXT = ".log";

FileShvJournal::FileShvJournal(FileShvJournal::SnapShotFn snf)
	: m_snapShotFn(snf)
	, m_journalDir("/tmp/shvjournal")
{
}

void FileShvJournal::append(const ShvJournalEntry &entry)
{
	shvLogFuncFrame() << "last file no:" << lastFileNo();
	try {
		int64_t last_msec = 0;
		int max_n = lastFileNo();
		if(max_n < 0)
			return;
		if(max_n == 0) {
			// journal dir is empty
			max_n++;
		}
		else {
			std::string fn = fileNoToName(max_n);
			last_msec = findLastEntryDateTime(fn);
			if(last_msec < 0) {
				// read date time error, log file might be corrupted, open new one
				max_n++;
			}
			else {
				auto fsz = fileSize(fn);
				shvDebug() << "\t current file size:" << fsz << "limit:" << m_fileSizeLimit;
				if(fsz > m_fileSizeLimit) {
					max_n++;
					shvDebug() << "\t file size limit exceeded, createing new file:" << max_n;
				}
			}
		}
		setLastFileNo(max_n);
		std::string fn = fileNoToName(max_n);
		shvDebug() << "\t appending records to file:" << fn;

		int64_t msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
		if(msec < last_msec)
			msec = last_msec;
		int uptime_sec = uptimeSec();

		std::ofstream out(fn, std::ios::binary | std::ios::out | std::ios::app);
		if(!out)
			throw std::runtime_error("Cannot open file " + fn + " for writing");

		long fsz = out.tellp();
		if(fsz == 0) {
			// new file should start with snapshot
			if(!m_snapShotFn)
				throw std::runtime_error("SnapShot function not defined");
			std::vector<ShvJournalEntry> snapshot;
			m_snapShotFn(snapshot);
			if(snapshot.empty())
				shvError() << "Empty snapshot created";
			for(const ShvJournalEntry &e : snapshot)
				appendEntry(out, msec, uptime_sec, e);
		}
		appendEntry(out, msec, uptime_sec, entry);
	}
	catch (std::exception &e) {
		shvError() << e.what();
		m_lastFileNo = -1;
	}
}

void FileShvJournal::appendEntry(std::ofstream &out, int64_t msec, int uptime_sec, const ShvJournalEntry &e)
{
	out << cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString();
	out << FIELD_SEPARATOR;
	out << uptime_sec;
	out << FIELD_SEPARATOR;
	out << e.path;
	out << FIELD_SEPARATOR;
	out << e.value.toCpon();
	out << RECORD_SEPARATOR;
	out.flush();
}

std::string FileShvJournal::fileNoToName(int n)
{
	char buff[10];
	std::snprintf(buff, sizeof (buff), "%04d", n);
	return m_journalDir + '/' + std::string(buff) + FILE_EXT;
}

long FileShvJournal::fileSize(const std::string &fn)
{
	std::ifstream in(fn, std::ios::in);
	if(!in)
		return -1;
	in.seekg(0, std::ios::end);
	return in.tellg();
}

int FileShvJournal::lastFileNo()
{
	if(m_lastFileNo > 0)
		return m_lastFileNo;
	int max_n = -1;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (m_journalDir.c_str())) != nullptr) {
		max_n = 0;
		std::string ext = FILE_EXT;
		while ((ent = readdir (dir)) != nullptr) {
			if(ent->d_type == DT_REG) {
				std::string fn = ent->d_name;
				if(!shv::core::String::endsWith(fn, ext))
					continue;
				try {
					int n = std::stoi(fn.substr(0, fn.size() - ext.size()));
					if(n > max_n)
						max_n = n;
				} catch (std::logic_error &e) {
					shvWarning() << "Mallformated shv journal file name" << fn << e.what();
				}
			}
		}
		closedir (dir);
	}
	else {
		throw std::runtime_error("Cannot read content of dir: " + m_journalDir);
	}
	m_lastFileNo = max_n;
	return m_lastFileNo;
}

int64_t FileShvJournal::findLastEntryDateTime(const std::string &fn)
{
	shvLogFuncFrame() << "'" + fn + "'";
	std::ifstream in(fn, std::ios::in | std::ios::binary);
	if (!in)
		throw std::runtime_error("Cannot open file: " + fn + " for reading.");
	int64_t dt_msec = -1;
	in.seekg(0, std::ios::end);
	long fpos = in.tellg();
	static constexpr int SKIP_LEN = 128;
	while(fpos > 0) {
		fpos -= SKIP_LEN;
		long chunk_len = SKIP_LEN;
		if(fpos < 0) {
			chunk_len += fpos;
			fpos = 0;
		}
		// date time string can be partialy on end of this chunk and at beggining of next,
		// read little bit more data to cover this
		// serialized date-time should never exceed 28 bytes see: 2018-01-10T12:03:56.123+0130
		chunk_len += 30;
		in.seekg(fpos, std::ios::beg);
		char buff[chunk_len];
		auto n = in.readsome(buff, sizeof(buff));
		if(n > 0) {
			std::string chunk(buff, static_cast<size_t>(n));
			size_t lf_pos = 0;
			while(lf_pos != std::string::npos) {
				lf_pos = chunk.find(RECORD_SEPARATOR, lf_pos);
				if(lf_pos != std::string::npos) {
					lf_pos++;
					auto tab_pos = chunk.find(FIELD_SEPARATOR, lf_pos);
					auto lf2_pos = chunk.find(RECORD_SEPARATOR, lf_pos);
					if(tab_pos != std::string::npos) {
						if(lf2_pos == std::string::npos || tab_pos < lf2_pos) {
							std::string s = chunk.substr(lf_pos, tab_pos - lf_pos);
							shvDebug() << "\t checking:" << s;
							chainpack::RpcValue::DateTime dt = chainpack::RpcValue::DateTime::fromUtcString(s);
							if(dt.isValid())
								dt_msec = dt.msecsSinceEpoch();
							else
								shvError() << "Malformed shv journal date time:" << s << "will be ignored.";
						}
						else {
							shvDebug() << "\t malformed line LF before TAB:" << chunk.substr(lf_pos, lf2_pos - lf_pos);;
						}
					}
					else if(chunk.size() - lf_pos > 0) {
						shvError() << "Truncated shv journal date time:" << chunk.substr(lf_pos) << "will be ignored.";
					}
				}
			}
			auto len = chunk.find(FIELD_SEPARATOR);
			if(len == std::string::npos) {
				break;
			}
			else {
			}
		}
		if(dt_msec > 0) {
			shvDebug() << "\t return:" << dt_msec << chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(dt_msec).toIsoString();
			return dt_msec;
		}
	}
	shvDebug() << "\t file does not containt record with valid date time";
	return -1;
}

} // namespace utils
} // namespace core
} // namespace shv
