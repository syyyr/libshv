#include "fileshvjournal.h"

#include <shv/chainpack/cponreader.h>
#include <shv/core/log.h>
#include <shv/core/string.h>
#include <shv/core/stringview.h>

#include <fstream>
#include <sstream>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define logWShvJournal() nCWarning("ShvJournal")
//#define logDShvJournal() nCDebug("ShvJournal")

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

#define SHV_STATBUF              struct stat64
#define SHV_STAT                 ::stat64
#define SHV_MKDIR                ::mkdir
#define SHV_REMOVE_FILE          ::remove

static bool mkpath(const std::string &dir_name)
{
	// helper function to check if a given path is a directory, since mkdir can
	// fail if the dir already exists (it may have been created by another
	// thread or another process)
	const auto is_dir = [](const std::string &dir_name) {
		SHV_STATBUF st;
		return SHV_STAT(dir_name.data(), &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
	};
	if(is_dir(dir_name))
		return true;
	if (SHV_MKDIR(dir_name.data(), 0777) == 0)
		return true;
	if (errno == EEXIST)
		return is_dir(dir_name);
	if (errno != ENOENT)
		return false;
	// mkdir failed because the parent dir doesn't exist, so try to create it
	auto const slash = dir_name.find_last_of('/');

	if (slash == std::string::npos)
		return false;
	std::string parent_dir_name = dir_name.substr(0, slash);
	if (!mkpath(parent_dir_name))
		return false;
	// try again
	if (SHV_MKDIR(dir_name.data(), 0777) == 0)
		return true;
	return errno == EEXIST && is_dir(dir_name);
}

static long file_size(const std::string &file_name)
{
	SHV_STATBUF st;
	if(SHV_STAT(file_name.data(), &st) == 0)
		return st.st_size;
	logWShvJournal() << "Cannot stat file:" << file_name;
	return -1;
}

static long rm_file(const std::string &file_name)
{
	long sz = file_size(file_name);
	if(SHV_REMOVE_FILE(file_name.c_str()) == 0)
		return sz;
	logWShvJournal() << "Cannot delete file:" << file_name;
	return 0;
}

ShvJournalGetLogParams::ShvJournalGetLogParams(const chainpack::RpcValue &opts)
	: ShvJournalGetLogParams()
{
	const cp::RpcValue::Map m = opts.toMap();
	since = m.value("since", since).toDateTime();
	if(!since.isValid())
		since = m.value("from").toDateTime();
	until = m.value("until", until).toDateTime();
	if(!until.isValid())
		until = m.value("to").toDateTime();
	pathPattern = m.value("pathPattern", pathPattern).toString();
	headerOptions = m.value("headerOptions", headerOptions).toUInt();
	maxRecordCount = m.value("maxRecordCount", maxRecordCount).toInt();
	withSnapshot = m.value("withSnapshot", withSnapshot).toBool();
}

chainpack::RpcValue ShvJournalGetLogParams::toRpcValue() const
{
	cp::RpcValue::Map m;
	if(since.isValid())
		m["since"] = since;
	if(until.isValid())
		m["until"] = until;
	if(!pathPattern.empty())
		m["pathPattern"] = pathPattern;
	m["headerOptions"] = headerOptions;
	m["maxRecordCount"] = maxRecordCount;
	m["withSnapshot"] = withSnapshot;
	return m;
}

const char * FileShvJournal::FILE_EXT = ".log";

FileShvJournal::FileShvJournal(FileShvJournal::SnapShotFn snf)
	: m_snapShotFn(snf)
	, m_journalDir("/tmp/shvjournal")
{
}

void FileShvJournal::append(const ShvJournalEntry &entry)
{
	shvLogFuncFrame();// << "last file no:" << lastFileNo();
	try {
		checkJournalDir();
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
				auto fsz = file_size(fn);
				shvDebug() << "\t current file size:" << fsz << "limit:" << m_fileSizeLimit;
				if(fsz > m_fileSizeLimit) {
					shvDebug() << "\t file size limit exceeded, createing new file:" << max_n;
					// rotate journal before creating new file
					rotateJournal();
					if(m_journalDirStatus.maxFileNo != max_n) {
						logWShvJournal() << "Max file no changed after log rotation, this should never happen !!!";
						max_n = m_journalDirStatus.maxFileNo;
					}
					max_n++;
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
				logWShvJournal() << "Empty snapshot created";
			for(const ShvJournalEntry &e : snapshot)
				appendEntry(out, msec, uptime_sec, e);
		}
		appendEntry(out, msec, uptime_sec, entry);
	}
	catch (std::exception &e) {
		logWShvJournal() << e.what();
		m_journalDirStatus.maxFileNo = -1;
	}
}

void FileShvJournal::appendEntry(std::ofstream &out, int64_t msec, int uptime_sec, const ShvJournalEntry &e)
{
	auto p1 = out.tellp();
	out << cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString();
	out << FIELD_SEPARATOR;
	out << uptime_sec;
	out << FIELD_SEPARATOR;
	out << e.path;
	out << FIELD_SEPARATOR;
	out << e.value.toCpon();
	out << RECORD_SEPARATOR;
	auto p2 = out.tellp();
	out.flush();
	m_journalDirStatus.journalSize += (p2 - p1);
}

void FileShvJournal::checkJournalDir()
{
	if(!mkpath(m_journalDir))
		throw std::runtime_error("Journal dir: " + m_journalDir + " do not exists and cannot be created");
}

void FileShvJournal::rotateJournal()
{
	updateJournalDirStatus();
	if(m_journalDirStatus.journalSize > m_dirSizeLimit) {
		for (int i = m_journalDirStatus.minFileNo; i < m_journalDirStatus.maxFileNo && m_journalDirStatus.journalSize > m_dirSizeLimit; ++i) {
			std::string fn = fileNoToName(i);
			m_journalDirStatus.journalSize -= rm_file(fn);
		}
		updateJournalDirStatus();
	}
}

std::string FileShvJournal::fileNoToName(int n)
{
	char buff[10];
	std::snprintf(buff, sizeof (buff), "%0*d", FILE_DIGITS, n);
	return m_journalDir + '/' + std::string(buff) + FILE_EXT;
}
/*
long FileShvJournal::fileSize(const std::string &fn)
{
	std::ifstream in(fn, std::ios::in);
	if(!in)
		return -1;
	in.seekg(0, std::ios::end);
	return in.tellg();
}
*/
int FileShvJournal::lastFileNo()
{
	if(m_journalDirStatus.maxFileNo < 0)
		updateJournalDirStatus();
	return m_journalDirStatus.maxFileNo;
}

void FileShvJournal::updateJournalDirStatus()
{
	int min_n = std::numeric_limits<int>::max();
	int max_n = -1;//std::numeric_limits<int>::min();
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (m_journalDir.c_str())) != nullptr) {
		max_n = 0;
		m_journalDirStatus.journalSize = 0;
		std::string ext = FILE_EXT;
		while ((ent = readdir (dir)) != nullptr) {
			if(ent->d_type == DT_REG) {
				std::string fn = ent->d_name;
				if(!shv::core::String::endsWith(fn, ext))
					continue;
				try {
					int n = std::stoi(fn.substr(0, fn.size() - ext.size()));
					if(n > 0) {
						if(n > max_n)
							max_n = n;
						if(n < min_n)
							min_n = n;
						std::string fn = m_journalDir + '/' + ent->d_name;
						m_journalDirStatus.journalSize += file_size(fn);
					}
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
	if(max_n == 0)
		min_n = 0;
	m_journalDirStatus.minFileNo = min_n;
	m_journalDirStatus.maxFileNo = max_n;
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
		in.read(buff, chunk_len);
		auto n = in.gcount();
		if(n > 0) {
			std::string chunk(buff, static_cast<size_t>(n));
			size_t lf_pos = 0;
			while(lf_pos != std::string::npos) {
				lf_pos = chunk.find(RECORD_SEPARATOR, lf_pos);
				if(lf_pos != std::string::npos) {
					lf_pos++;
					auto tab_pos = chunk.find(FIELD_SEPARATOR, lf_pos);
					if(tab_pos != std::string::npos) {
						std::string s = chunk.substr(lf_pos, tab_pos - lf_pos);
						shvDebug() << "\t checking:" << s;
						chainpack::RpcValue::DateTime dt = chainpack::RpcValue::DateTime::fromUtcString(s);
						if(dt.isValid())
							dt_msec = dt.msecsSinceEpoch();
						else
							logWShvJournal() << fn << "Malformed shv journal date time:" << s << "will be ignored.";
					}
					else if(chunk.size() - lf_pos > 0) {
						logWShvJournal() << fn << "Truncated shv journal date time:" << chunk.substr(lf_pos) << "will be ignored.";
					}
				}
			}
		}
		if(dt_msec > 0) {
			shvDebug() << "\t return:" << dt_msec << chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(dt_msec).toIsoString();
			return dt_msec;
		}
	}
	logWShvJournal() << fn << "File does not containt record with valid date time";
	return -1;
}

chainpack::RpcValue FileShvJournal::getLog(const ShvJournalGetLogParams &params)
{
	shvLogFuncFrame() << params.toRpcValue().toCpon();
	checkJournalDir();
	auto since_msec = params.since.isValid()? params.since.msecsSinceEpoch(): 0;
	//auto until_msec = until.isValid()? until.msecsSinceEpoch(): std::numeric_limits<int64_t>::max();
	int last_file_no = lastFileNo();
	int min_file_no = 0;
	int64_t min_msec = 0;
	int file_no = last_file_no;
	for(; file_no > 0 && file_no >= m_journalDirStatus.minFileNo; file_no--) {
		std::string fn = fileNoToName(file_no);
		std::ifstream in(fn, std::ios::in | std::ios::binary);
		if (!in) {
			shvDebug() << "Cannot open file: " + fn + " for reading, log file missing.";
			continue;
		}
		min_file_no = file_no;
		char buff[32];
		in.read(buff, sizeof(buff));
		auto n = in.gcount();
		if(n > 0) {
			std::string s(buff, (size_t)n);
			chainpack::RpcValue::DateTime dt = chainpack::RpcValue::DateTime::fromUtcString(s);
			if(dt.isValid()) {
				auto dt_msec = dt.msecsSinceEpoch();
				min_msec = dt_msec;
				if(dt_msec <= since_msec) {
					break;
				}
			}
			else {
				logWShvJournal() << fn << "Malformed shv journal date time:" << s << "will be ignored.";
			}
		}
		else {
			logWShvJournal() << "Cannot read date time string from: " + fn;
		}
	}
	cp::RpcValue::List log;
	if(!params.since.isValid()) {
		file_no = min_file_no;
		since_msec = min_msec;
	}
	if(file_no < m_journalDirStatus.minFileNo)
		file_no = m_journalDirStatus.minFileNo;

	// this ensure that there be only one copy of each path in memory
	cp::RpcValue::Map path_cache;
	unsigned max_path_id = 0;
	auto make_path_shared = [&path_cache, &max_path_id, &params](const std::string &path) -> cp::RpcValue {
		cp::RpcValue ret = path_cache.value(path);
		if(ret.isValid())
			return ret;
		if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::PathsDict))
			ret = ++max_path_id;
		else
			ret = path;
		path_cache[path] = ret;
		return ret;
	};
	int rec_cnt = 0;
	if(file_no > 0) {
		std::map<std::string, std::string> snapshot;
		for(; file_no <= last_file_no; file_no++) {
			std::string fn = fileNoToName(file_no);
			shvDebug() << "======================= opening file:" << fn;
			std::ifstream in(fn, std::ios::in | std::ios::binary);
			if(!in) {
				logWShvJournal() << "Cannot open file: " + fn + " for reading, log file missing.";
				continue;
			}
			while(in) {
				std::string line = getLine(in, RECORD_SEPARATOR);
				shvDebug() << "line:" << line;
				std::istringstream iss(line);
				std::string dtstr = getLine(iss, FIELD_SEPARATOR);
				if(dtstr.empty()) {
					shvDebug() << "\t skipping empty line";
					continue; // skip empty line
				}
				std::string upstr = getLine(iss, FIELD_SEPARATOR);
				std::string path = getLine(iss, FIELD_SEPARATOR);
				std::string valstr = getLine(iss, FIELD_SEPARATOR);
				if(!params.pathPattern.empty() && !pathMatch(params.pathPattern, path))
					continue;
				cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromUtcString(dtstr);
				if(!dt.isValid()) {
					logWShvJournal() << fn << "invalid date time string:" << dtstr;
					continue;
				}
				shvDebug() << "\t FIELDS:" << dtstr << '\t' << path << '\t' << valstr;
				if(dt < params.since) {
					if(params.withSnapshot) {
						snapshot[path] = std::move(valstr);
					}
				}
				else if(!params.until.isValid() || dt <= params.until) {
					if(params.withSnapshot && !snapshot.empty()) {
						shvDebug() << "\t -------------- Snapshot";
						for(const auto &kv : snapshot) {
							cp::RpcValue::List rec;
							rec.push_back(params.since);
							//rec.push_back(0);
							rec.push_back(make_path_shared(kv.first));
							rec.push_back(cp::RpcValue::fromCpon(kv.second));
							log.push_back(rec);
							rec_cnt++;
						}
						snapshot.clear();
					}
					cp::RpcValue::List rec;
					rec.push_back(cp::RpcValue::DateTime::fromUtcString(dtstr));
					//rec.push_back(toLong(upstr));
					rec.push_back(make_path_shared(path));
					rec.push_back(cp::RpcValue::fromCpon(valstr));
					shvDebug() << "\t LOG:" << rec[0].toDateTime().toIsoString() << '\t' << path << '\t' << rec[2].toCpon();
					log.push_back(rec);
					rec_cnt++;
					if(rec_cnt >= params.maxRecordCount)
						goto log_finish;
				}
				else {
					goto log_finish;
				}
			}
		}
	}
log_finish:
	cp::RpcValue ret = log;
	cp::RpcValue::MetaData md;
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::BasicInfo)) {
		if(!m_deviceId.empty()) {
			cp::RpcValue::Map device;
			device["id"] = m_deviceId;
			md.setValue("device", device); // required
		}
		md.setValue("logVersion", 1); // required
		md.setValue("dateTime", cp::RpcValue::DateTime::now());
		md.setValue("params", params.toRpcValue());
	}
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::FileldInfo)) {
		md.setValue("fields", cp::RpcValue::List{
						cp::RpcValue::Map{{"name", "timestamp"}},
						cp::RpcValue::Map{{"name", "path"}},
						cp::RpcValue::Map{{"name", "value"}},
					});
	}
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::TypeInfo)) {
		if(m_typeInfo.isValid())
			md.setValue("typeInfo", m_typeInfo);
	}
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::PathsDict)) {
		cp::RpcValue::IMap path_dict;
		for(auto kv : path_cache) {
			path_dict[kv.second.toInt()] = kv.first;
		}
		md.setValue("pathsDict", path_dict);
	}
	if(!md.isEmpty())
		ret.setMetaData(std::move(md));
	return ret;
}

std::string FileShvJournal::getLine(std::istream &in, char sep)
{
	std::string line;
	while(in) {
		char buff[1024];
		in.getline(buff, sizeof (buff), sep);
		std::string s(buff);
		line += s;
		if(in.gcount() == (long)s.size()) {
			// LF not found
			continue;
		}
		break;
	}
	return line;
}

long FileShvJournal::toLong(const std::string &s)
{
	try {
		return std::stol(s);
	} catch (std::logic_error &e) {
		shvError() << s << "cannot be converted to int:" << e.what();
	}
	return 0;
}

bool FileShvJournal::pathMatch(const std::string &pattern, const std::string &path)
{
	const shv::core::StringViewList ptlst = shv::core::StringView(pattern).split('/');
	const shv::core::StringViewList phlst = shv::core::StringView(path).split('/');
	size_t ptix = 0;
	size_t phix = 0;
	while(true) {
		if(phix == phlst.size() && ptix == ptlst.size())
			return true;
		if(ptix == ptlst.size() && phix < phlst.size())
			return false;
		if(phix == phlst.size() && ptix == ptlst.size() - 1 && ptlst[ptix] == "**")
			return true;
		if(phix == phlst.size() && ptix < ptlst.size())
			return false;
		const shv::core::StringView &pt = ptlst[ptix];
		if(pt == "*") {
			// match exactly one path segment
		}
		else if(pt == "**") {
			// match zero or more path segments
			ptix++;
			if(ptix == ptlst.size())
				return true;
			const shv::core::StringView &pt2 = ptlst[ptix];
			do {
				const shv::core::StringView &ph = phlst[phix];
				if(ph == pt2)
					break;
				phix++;
			} while(phix < phlst.size());
			if(phix == phlst.size())
				return false;
		}
		else {
			const shv::core::StringView &ph = phlst[phix];
			if(!(ph == pt))
				return false;
		}
		ptix++;
		phix++;
	}
}

} // namespace utils
} // namespace core
} // namespace shv
