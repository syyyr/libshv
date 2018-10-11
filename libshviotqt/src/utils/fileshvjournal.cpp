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
				if(fsz > m_fileSizeLimit) {
					max_n++;
				}
			}
		}
		setLastFileNo(max_n);
		std::string fn = fileNoToName(max_n);

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
		if(fsz >= 0)
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
	std::ifstream in(fn, std::ios::in);
	if (in) {
		int64_t dt_msec = -1;
		in.seekg(0, std::ios::end);
		//long fpos = in.tellg();
		static constexpr int SKIP_LEN = 64;
		while(true) {
			in.seekg(-SKIP_LEN, std::ios::cur);
			bool last_try = in.tellg() == 0;
			for (int i = 0; i < SKIP_LEN; ++i) {
				int c = in.get();
				if(c < 0)
					break;
				if(c == RECORD_SEPARATOR) {
					cp::CponReader rd(in);
					chainpack::RpcValue::DateTime dt = rd.readDateTime();
					if(dt.isValid())
						dt_msec = dt.msecsSinceEpoch();
				}
			}
			if(dt_msec > 0)
				return dt_msec;
			if(last_try)
				return -1;
		}
	}
	throw std::runtime_error("Cannot open file: " + fn + " for reading.");
}

} // namespace utils
} // namespace core
} // namespace shv
