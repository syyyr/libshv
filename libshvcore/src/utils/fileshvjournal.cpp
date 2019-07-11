#include "fileshvjournal.h"
#include "shvpath.h"

#include "../log.h"
#include "../string.h"
#include "../stringview.h"

#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpc.h>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <regex>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

static int uptimeSec()
{
	int uptime;
	if (std::ifstream("/proc/uptime", std::ios::in) >> uptime) {
		return uptime;
	}
	return 0;
}
/*
static uint16_t shortTime()
{
	std::chrono::time_point<std::chrono::system_clock> p1 = std::chrono::system_clock::now();
	int64_t msecs = std::chrono::duration_cast<std::chrono:: milliseconds>(p1.time_since_epoch()).count();
	return static_cast<uint16_t>(msecs % 65536);
}
*/
#define SHV_STATBUF              struct stat64
#define SHV_STAT                 ::stat64
#ifdef __unix
#define SHV_MKDIR(dir_name)      ::mkdir(dir_name, 0777)
#else
#define SHV_MKDIR(dir_name)      ::mkdir(dir_name)
#endif
#define SHV_REMOVE_FILE          ::remove

static bool is_dir(const std::string &dir_name)
{
	SHV_STATBUF st;
	return SHV_STAT(dir_name.data(), &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR;
}

static bool mkpath(const std::string &dir_name)
{
	// helper function to check if a given path is a directory, since mkdir can
	// fail if the dir already exists (it may have been created by another
	// thread or another process)
	if(is_dir(dir_name))
		return true;
	if (SHV_MKDIR(dir_name.data()) == 0)
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
	if (SHV_MKDIR(dir_name.data()) == 0)
		return true;
	return errno == EEXIST && is_dir(dir_name);
}

static int64_t file_size(const std::string &file_name)
{
	SHV_STATBUF st;
	if(SHV_STAT(file_name.data(), &st) == 0)
		return st.st_size;
	logWShvJournal() << "Cannot stat file:" << file_name;
	return -1;
}

static bool file_exists(const std::string &file_name)
{
	SHV_STATBUF st;
	return (SHV_STAT(file_name.data(), &st) == 0);
}

static int64_t rm_file(const std::string &file_name)
{
	int64_t sz = file_size(file_name);
	if(SHV_REMOVE_FILE(file_name.c_str()) == 0)
		return sz;
	logWShvJournal() << "Cannot delete file:" << file_name;
	return 0;
}

static int64_t str_to_size(const std::string &str)
{
	std::istringstream is(str);
	int64_t n;
	char c;
	is >> n >> c;
	switch(std::toupper(c)) {
	case 'K': n *= 1024; break;
	case 'M': n *= 1024 * 1024; break;
	case 'G': n *= 1024 * 1024 * 1024; break;
	}
	if(n < 1024)
		n = 1024;
	return n;
}

//==============================================================
// ShvJournalEntry
//==============================================================
const char *ShvJournalEntry::DOMAIN_VAL_CHANGE = "C";
const char *ShvJournalEntry::DOMAIN_VAL_FASTCHANGE = "F";
const char *ShvJournalEntry::DOMAIN_VAL_SERVICECHANGE = "S";

//==============================================================
// FileShvJournal
//==============================================================
const char * FileShvJournal::FILE_EXT = ".log";
const char * FileShvJournal::KEY_NAME = "name";
const char *FileShvJournal::KEY_RECORD_COUNT = "recordCount";
const char *FileShvJournal::KEY_PATHS_DICT = "pathsDict";

FileShvJournal::FileShvJournal(std::string device_id, FileShvJournal::SnapShotFn snf)
	: m_deviceId(std::move(device_id))
	, m_snapShotFn(snf)
{
}

void FileShvJournal::setJournalDir(std::string s)
{
	m_journalDir = std::move(s);
}

const std::string &FileShvJournal::journalDir()
{
	if(m_journalDir.empty()) {
		std::string d = "/tmp/shvjournal/";
		if(m_deviceId.empty()) {
			d += "default";
		}
		else {
			std::string id = m_deviceId;
			String::replace(id, '/', '-');
			String::replace(id, ':', '-');
			String::replace(id, '.', '-');
			d += id;
		}
		m_journalDir = d;
		shvWarning() << "Journal dir not set, falling back to default value:" << journalDir();
	}
	return m_journalDir;
}

void FileShvJournal::setFileSizeLimit(const std::string &n)
{
	setFileSizeLimit(str_to_size(n));
}

void FileShvJournal::setJournalSizeLimit(const std::string &n)
{
	setJournalSizeLimit(str_to_size(n));
}

void FileShvJournal::append(const ShvJournalEntry &entry, int64_t msec)
{
	shvLogFuncFrame();// << "last file no:" << lastFileNo();
	try {
		checkJournalConsistecy();
		std::string fn = fileNoToName(m_journalStatus.maxFileNo);
		shvDebug() << "\t appending records to file:" << fn;

		if(msec == 0)
			msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
		if(msec < m_journalStatus.recentTimeStamp)
			msec = m_journalStatus.recentTimeStamp;
		{
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
					appendEntry(out, msec, e);
			}
			appendEntry(out, msec, entry);
			ssize_t file_size = out.tellp();
			m_journalStatus.journalSize += (file_size - fsz);
			if(fsz > m_fileSizeLimit) {
				m_journalStatus.maxFileNo++;
			}
		}
		if(m_journalStatus.journalSize > m_journalSizeLimit) {
			rotateJournal();
		}
	}
	catch (std::exception &e) {
		shvWarning() << e.what();
		m_journalStatus.maxFileNo = -1;
	}
}

void FileShvJournal::appendEntry(std::ofstream &out, int64_t msec, const ShvJournalEntry &e)
{
	out << cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString();
	out << FIELD_SEPARATOR;
	out << uptimeSec();
	out << FIELD_SEPARATOR;
	out << e.path;
	out << FIELD_SEPARATOR;
	out << e.value.toCpon();
	out << FIELD_SEPARATOR;
	if(e.shortTime >= 0)
		out << e.shortTime;
	out << RECORD_SEPARATOR;
	out.flush();
}

void FileShvJournal::checkJournalConsistecy()
{
	if(!m_journalStatus.isConsistent()) {
		checkJournalDir();
		updateJournalStatus();
	}
	if(!m_journalStatus.isConsistent()) {
		throw std::runtime_error("Journal cannot be brought to consistent state.");
	}
}

void FileShvJournal::checkJournalDir()
{
	if(!mkpath(m_journalDir)) {
		m_journalStatus.journalDirExists = false;
		throw std::runtime_error("Journal dir: " + m_journalDir + " do not exists and cannot be created");
	}
	m_journalStatus.journalDirExists = true;
}

void FileShvJournal::rotateJournal()
{
	updateJournalStatus();
	for (int i = m_journalStatus.minFileNo; i < m_journalStatus.maxFileNo && m_journalStatus.journalSize > m_journalSizeLimit; ++i) {
		std::string fn = fileNoToName(i);
		m_journalStatus.journalSize -= rm_file(fn);
	}
	updateJournalStatus();
}

std::string FileShvJournal::fileNoToName(int n)
{
	char buff[10];
	std::snprintf(buff, sizeof (buff), "%0*d", FILE_DIGITS, n);
	return m_journalDir + '/' + std::string(buff) + FILE_EXT;
}
#ifdef __unix
#define DIRENT_HAS_TYPE_FIELD
#endif
void FileShvJournal::updateJournalStatus()
{
	int min_n = std::numeric_limits<int>::max();
	int max_n = -1;//std::numeric_limits<int>::min();
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (m_journalDir.c_str())) != nullptr) {
		max_n = 0;
		m_journalStatus.journalSize = 0;
		std::string ext = FILE_EXT;
		while ((ent = readdir (dir)) != nullptr) {
#ifdef DIRENT_HAS_TYPE_FIELD
			if(ent->d_type == DT_REG) {
#endif
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
						m_journalStatus.journalSize += file_size(fn);
					}
				} catch (std::logic_error &e) {
					shvWarning() << "Malformed shv journal file name: " << fn << e.what();
				}
#ifdef DIRENT_HAS_TYPE_FIELD
			}
#endif
		}
		closedir (dir);
	}
	else {
		throw std::runtime_error("Cannot read content of dir: " + m_journalDir);
	}
	if(max_n == 0)
		min_n = 0;
	m_journalStatus.minFileNo = min_n;
	m_journalStatus.maxFileNo = max_n;
	if(m_journalStatus.maxFileNo == 0) {
		m_journalStatus.maxFileNo++;
		m_journalStatus.recentTimeStamp = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	}
	else {
		std::string fn = fileNoToName(m_journalStatus.maxFileNo);
		m_journalStatus.recentTimeStamp = findLastEntryDateTime(fn);
		if(m_journalStatus.recentTimeStamp < 0) {
			// corrupted file, start new one
			m_journalStatus.maxFileNo++;
			m_journalStatus.recentTimeStamp = cp::RpcValue::DateTime::now().msecsSinceEpoch();
		}
	}
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
						size_t len;
						chainpack::RpcValue::DateTime dt = chainpack::RpcValue::DateTime::fromUtcString(s, &len);
						if(len > 0)
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
	logDShvJournal() << "========================= getLog ==================" << params.toRpcValue().toCpon();
	logDShvJournal() << "params:" << params.toRpcValue().toCpon();
	checkJournalConsistecy();
	auto since_msec = params.since.isValid()? params.since.toDateTime().msecsSinceEpoch(): 0;
	int min_file_no = 0;
	int64_t min_msec = 0;
	int file_no = m_journalStatus.maxFileNo;
	for(; file_no > 0 && file_no >= m_journalStatus.minFileNo; file_no--) {
		std::string fn = fileNoToName(file_no);
		logDShvJournal() << "checking file:" << fn;
		std::ifstream in(fn, std::ios::in | std::ios::binary);
		if (!in) {
			logWShvJournal() << "Cannot open file: " + fn + " for reading, log file missing.";
			continue;
		}
		min_file_no = file_no;
		char buff[32];
		in.read(buff, sizeof(buff));
		auto n = in.gcount();
		if(n > 0) {
			std::string s(buff, (size_t)n);
			size_t len;
			chainpack::RpcValue::DateTime dt = chainpack::RpcValue::DateTime::fromUtcString(s, &len);
			logDShvJournal() << "\tfirst timestamp:" << dt.toIsoString();
			if(len > 0) {
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
	if(!params.since.isDateTime()) {
		file_no = min_file_no;
		since_msec = min_msec;
	}
	if(file_no < m_journalStatus.minFileNo)
		file_no = m_journalStatus.minFileNo;

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
		int max_rec_cnt = std::min(params.maxRecordCount, m_getLogRecordCountLimit);
		struct SnapshotEntry
		{
			std::string uptime;
			std::string value;
			std::string shorttime;
		};
		std::map<std::string, SnapshotEntry> snapshot;
		for(; file_no <= m_journalStatus.maxFileNo; file_no++) {
			std::string fn = fileNoToName(file_no);
			logDShvJournal() << "---------------------------------- opening file:" << fn;
			std::ifstream in(fn, std::ios::in | std::ios::binary);
			if(!in) {
				logWShvJournal() << "Cannot open file: " + fn + " for reading, log file missing.";
				continue;
			}
			while(in) {
				std::string line = getLine(in, RECORD_SEPARATOR);
				//logDShvJournal() << "line:" << line;
				shv::core::StringView sv(line);
				shv::core::StringViewList lst = sv.split(FIELD_SEPARATOR, shv::core::StringView::KeepEmptyParts);
				if(lst.empty()) {
					logDShvJournal() << "\t skipping empty line";
					continue; // skip empty line
				}
				std::string dtstr = lst[Column::Timestamp].toString();
				ShvPath path = lst.value(Column::Path).toString();
				if(!params.pathPattern.empty() && !path.matchWild(params.pathPattern))
					continue;
				size_t len;
				cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromUtcString(dtstr, &len);
				if(len == 0) {
					logWShvJournal() << fn << "invalid date time string:" << dtstr;
					continue;
				}
				logDShvJournal() << "\t FIELDS:" << dtstr << '\t' << path << "vals:" << lst.join('|');
				if(dt < params.since.toDateTime()) {
					if(params.withSnapshot)
						snapshot[path] = SnapshotEntry{
								lst.value(Column::UpTime).toString(),
								lst.value(Column::Value).toString(),
								lst.value(Column::ShortTime).toString()};
				}
				else {
					if(params.withSnapshot && !snapshot.empty()) {
						logDShvJournal() << "\t -------------- Snapshot";
						std::string err;
						for(const auto &kv : snapshot) {
							cp::RpcValue::List rec;
							rec.push_back(params.since);
							if(params.withUptime)
								rec.push_back(cp::RpcValue::fromCpon(kv.second.uptime, &err));
							rec.push_back(make_path_shared(kv.first));
							rec.push_back(cp::RpcValue::fromCpon(kv.second.value, &err));
							std::string short_time_str = kv.second.shorttime;
							if(!short_time_str.empty())
								rec.push_back(cp::RpcValue::fromCpon(short_time_str, &err));
							log.push_back(rec);
							rec_cnt++;
							if(rec_cnt >= max_rec_cnt)
								goto log_finish;
						}
						snapshot.clear();
					}
					if(!params.until.isDateTime() || dt <= params.until.toDateTime()) {
						std::string err;
						cp::RpcValue::List rec;
						rec.push_back(dt);
						if(params.withUptime)
							rec.push_back(cp::RpcValue::fromCpon(lst.value(Column::UpTime).toString(), &err));
						rec.push_back(make_path_shared(path));
						rec.push_back(cp::RpcValue::fromCpon(lst.value(Column::Value).toString(), &err));
						std::string short_time_str = lst.value(Column::ShortTime).toString();
						if(!short_time_str.empty())
							rec.push_back(cp::RpcValue::fromCpon(short_time_str, &err));
						//logDShvJournal() << "\t LOG:" << rec[Column::Timestamp].toDateTime().toIsoString() << '\t' << path << '\t' << rec[2].toCpon();
						log.push_back(rec);
						rec_cnt++;
						if(rec_cnt >= max_rec_cnt)
							goto log_finish;
					}
					else {
						goto log_finish;
					}
				}
			}
		}
	}
log_finish:
	cp::RpcValue ret = log;
	cp::RpcValue::MetaData md;
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::BasicInfo)) {
		{
			cp::RpcValue::Map device;
			device["id"] = m_deviceId;
			device["type"] = m_deviceType;
			md.setValue("device", device); // required
		}
		md.setValue("logVersion", 1); // required
		md.setValue("dateTime", cp::RpcValue::DateTime::now());
		md.setValue("params", params.toRpcValue());
		md.setValue(KEY_RECORD_COUNT, rec_cnt);
	}
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::FieldInfo)) {
		cp::RpcValue::List fields;
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Timestamp)}});
		if(params.withUptime)
			fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::UpTime)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Path)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Value)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::ShortTime)}});
		md.setValue("fields", std::move(fields));
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
		md.setValue(KEY_PATHS_DICT, path_dict);
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
			// separator not found
			continue;
		}
		break;
	}
	return line;
}
/*
long FileShvJournal::toLong(const std::string &s)
{
	try {
		return std::stol(s);
	} catch (std::logic_error &e) {
		shvError() << s << "cannot be converted to int:" << e.what();
	}
	return 0;
}
*/
const char *FileShvJournal::Column::name(FileShvJournal::Column::Enum e)
{
	switch (e) {
	case Column::Enum::Timestamp: return "timestamp";
	case Column::Enum::UpTime: return "upTime";
	case Column::Enum::Path: return "path";
	case Column::Enum::Value: return "value";
	case Column::Enum::ShortTime: return "shortTime";
	}
	return "invalid";
}

//==============================================================
// FileShvJournal2
//==============================================================
const char * FileShvJournal2::KEY_NAME = "name";
const char *FileShvJournal2::KEY_RECORD_COUNT = "recordCount";
const char *FileShvJournal2::KEY_PATHS_DICT = "pathsDict";

static constexpr size_t MIN_SEP_POS = 13;
static constexpr size_t SEC_SEP_POS = MIN_SEP_POS + 3;
static constexpr size_t MSEC_SEP_POS = SEC_SEP_POS + 3;

FileShvJournal2::FileShvJournal2(std::string device_id, FileShvJournal2::SnapShotFn snf)
	: m_deviceId(std::move(device_id))
	, m_snapShotFn(snf)
{
	setFileExtension(".log2");
}

void FileShvJournal2::setJournalDir(std::string s)
{
	m_journalDir = std::move(s);
}

const std::string &FileShvJournal2::journalDir()
{
	if(m_journalDir.empty()) {
		std::string d = "/tmp/shvjournal/";
		if(m_deviceId.empty()) {
			d += "default";
		}
		else {
			std::string id = m_deviceId;
			String::replace(id, '/', '-');
			String::replace(id, ':', '-');
			String::replace(id, '.', '-');
			d += id;
		}
		m_journalDir = d;
		shvWarning() << "Journal dir not set, falling back to default value:" << journalDir();
	}
	return m_journalDir;
}

void FileShvJournal2::setFileSizeLimit(const std::string &n)
{
	setFileSizeLimit(str_to_size(n));
}

void FileShvJournal2::setJournalSizeLimit(const std::string &n)
{
	setJournalSizeLimit(str_to_size(n));
}

void FileShvJournal2::append(const ShvJournalEntry &entry, int64_t msec)
{
	try {
		appendThrow(entry, msec);
		return;
	}
	catch (std::exception &e) {
		logIShvJournal() << "Append to log failed, journal dir will be read again, SD card might be replaced:" << e.what();
	}
	try {
		checkJournalConsistecy(true);
		appendThrow(entry, msec);
	}
	catch (std::exception &e) {
		logWShvJournal() << "Append to log failed after journal dir check:" << e.what();
	}
}

void FileShvJournal2::appendThrow(const ShvJournalEntry &entry, int64_t msec)
{
	shvLogFuncFrame();// << "last file no:" << lastFileNo();
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	/// keep journal monotonic
	if(msec < m_journalStatus.recentTimeStamp)
		msec = m_journalStatus.recentTimeStamp;

	ensureJournalDir();
	checkJournalConsistecy();

	int64_t last_file_msec = 0;
	if(m_journalStatus.files.empty()) {
		last_file_msec = msec;
	}
	else if(m_journalStatus.lastFileSize > m_fileSizeLimit) {
		last_file_msec = m_journalStatus.files[m_journalStatus.files.size() - 1];
		if(msec > last_file_msec) {
			/// create new file when it has different name only
			last_file_msec = msec;
		}
	}
	else {
		last_file_msec = m_journalStatus.files[m_journalStatus.files.size() - 1];
	}
	std::string fn = fileMsecToFilePath(last_file_msec);
	{
		//logDShvJournal() << "\t appending records to file:" << fn;
		std::ofstream out(fn, std::ios::binary | std::ios::out | std::ios::app);
		if(!out)
			throw std::runtime_error("Cannot open file " + fn + " for writing");

		long fsz = out.tellp();
		if(fsz == 0) {
			// new file should start with snapshot
			logDShvJournal() << "\t new file, snapshot will be written to:" << fn;
			if(!m_snapShotFn)
				throw std::runtime_error("SnapShot function not defined");
			std::vector<ShvJournalEntry> snapshot;
			m_snapShotFn(snapshot);
			if(snapshot.empty())
				logWShvJournal() << "Empty snapshot created";
			for(const ShvJournalEntry &e : snapshot)
				appendEntry(out, msec, e);
			m_journalStatus.files.push_back(last_file_msec);
		}
		appendEntry(out, msec, entry);
		ssize_t file_size = out.tellp();
		m_journalStatus.lastFileSize = file_size;
		m_journalStatus.journalSize += file_size - fsz;;
	}
	if(m_journalStatus.journalSize > m_journalSizeLimit) {
		rotateJournal();
	}
}

void FileShvJournal2::appendEntry(std::ofstream &out, int64_t msec, const ShvJournalEntry &e)
{
	/*
	logDShvJournal() << "\t appending entry:"
					 << cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString()
					 << uptimeSec()
					 << e.path
					 << e.value.toCpon()
					 << e.shortTime;
	*/
	out << cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString();
	out << FIELD_SEPARATOR;
	out << uptimeSec();
	out << FIELD_SEPARATOR;
	out << e.path;
	out << FIELD_SEPARATOR;
	out << e.value.toCpon();
	out << FIELD_SEPARATOR;
	if(e.shortTime >= 0)
		out << e.shortTime;
	out << FIELD_SEPARATOR;
	out << e.domain;
	out << RECORD_SEPARATOR;
	out.flush();
	m_journalStatus.recentTimeStamp = msec;
}

int64_t FileShvJournal2::fileNameToFileMsec(const std::string &fn) const
{
	std::string utc_str = fn;
	if(MSEC_SEP_POS >= utc_str.size())
		throw std::runtime_error("fn2msec(): File name: '" + fn + "' too short.");
	utc_str[MIN_SEP_POS] = ':';
	utc_str[SEC_SEP_POS] = ':';
	utc_str[MSEC_SEP_POS] = '.';
	int64_t msec = cp::RpcValue::DateTime::fromUtcString(utc_str).msecsSinceEpoch();
	if(msec == 0)
		throw std::runtime_error("fn2msec(): Invalid file name: '" + fn + "' cannot be converted to date time");
	return msec;
}

std::string FileShvJournal2::fileMsecToFileName(int64_t msec) const
{
	std::string fn = cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString(cp::RpcValue::DateTime::MsecPolicy::Always, false);
	fn[MIN_SEP_POS] = '-';
	fn[SEC_SEP_POS] = '-';
	fn[MSEC_SEP_POS] = '-';
	return fn + fileExtensionRef();
}

std::string FileShvJournal2::fileMsecToFilePath(int64_t file_msec) const
{
	std::string fn = fileMsecToFileName(file_msec);
	return m_journalDir + '/' + fn;
}

void FileShvJournal2::checkJournalConsistecy(bool force)
{
	if(!m_journalStatus.isConsistent() || force) {
		logDShvJournal() << "journal status not consistent or check forced";
		m_journalStatus.journalDirExists = journalDirExists();
		if(m_journalStatus.journalDirExists)
			updateJournalStatus();
		else
			shvWarning() << "Journal dir:" << journalDir() << "does not exist!";
	}
	if(!m_journalStatus.isConsistent()) {
		throw std::runtime_error("Journal cannot be brought to consistent state.");
	}
}

void FileShvJournal2::ensureJournalDir()
{
	if(!mkpath(m_journalDir)) {
		m_journalStatus.journalDirExists = false;
		throw std::runtime_error("Journal dir: " + m_journalDir + " do not exists and cannot be created");
	}
	//logDShvJournal() << "journal dir:" << m_journalDir << "exists";
	m_journalStatus.journalDirExists = true;
}

bool FileShvJournal2::journalDirExists()
{
	return is_dir(journalDir());
}

void FileShvJournal2::rotateJournal()
{
	updateJournalFiles();
	size_t file_cnt = m_journalStatus.files.size();
	for(int64_t file_msec : m_journalStatus.files) {
		if(file_cnt == 1) {
			/// keep at least one file in case of bad limits configuration
			break;
		}
		if(m_journalStatus.journalSize < m_journalSizeLimit)
			break;
		std::string fn = fileMsecToFilePath(file_msec);
		m_journalStatus.journalSize -= rm_file(fn);
		file_cnt--;
	}
	updateJournalStatus();
}

void FileShvJournal2::convertLog1JournalDir()
{
	const std::string &journal_dir = journalDir();
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (journal_dir.c_str())) != nullptr) {
		const std::string ext = ".log";
		int n_files = 0;
		while ((ent = readdir (dir)) != nullptr) {
#ifdef DIRENT_HAS_TYPE_FIELD
			if(ent->d_type == DT_REG) {
#endif
				std::string fn = ent->d_name;
				if(!shv::core::String::endsWith(fn, ext))
					continue;
				if(n_files++ == 0)
					shvInfo() << "======= Journal1 format file(s) found, converting to format 2";
				int n = 0;
				try {
					n = std::stoi(fn.substr(0, fn.size() - ext.size()));
				} catch (std::logic_error &e) {
					shvWarning() << "Malformed shv journal file name" << fn << e.what();
				}
				if(n > 0) {
					fn = journal_dir + '/' + ent->d_name;
					std::ifstream in(fn, std::ios::in | std::ios::binary);
					if (!in) {
						shvWarning() << "Cannot open file:" << fn << "for reading.";
					}
					else {
						static constexpr size_t DT_LEN = 30;
						char buff[DT_LEN];
						in.read(buff, sizeof (buff));
						auto n = in.gcount();
						if(n > 0) {
							std::string s(buff, (unsigned)n);
							int64_t file_msec = chainpack::RpcValue::DateTime::fromUtcString(s).msecsSinceEpoch();
							if(file_msec == 0) {
								shvWarning() << "cannot read date time from first line of file:" << fn << "line:" << s;
							}
							else {
								std::string new_fn = journal_dir + '/' + fileMsecToFileName(file_msec);
								shvInfo() << "renaming" << fn << "->" << new_fn;
								if (std::rename(fn.c_str(), new_fn.c_str())) {
									shvError() << "cannot rename:" << fn << "to:" << new_fn;
								}
							}
						}
					}
				}
#ifdef DIRENT_HAS_TYPE_FIELD
			}
#endif
		}
		closedir (dir);
	}
	else {
		shvError() << "Cannot read content of dir:" << journal_dir;
	}
}

#ifdef __unix
#define DIRENT_HAS_TYPE_FIELD
#endif
void FileShvJournal2::updateJournalStatus()
{
	updateJournalFiles();
	updateRecentTimeStamp();
}

void FileShvJournal2::updateJournalFiles()
{
	m_journalStatus.journalSize = 0;
	m_journalStatus.lastFileSize = 0;
	m_journalStatus.files.clear();
	int64_t max_file_msec = -1;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (m_journalDir.c_str())) != nullptr) {
		m_journalStatus.journalSize = 0;
		const std::string &ext = fileExtensionRef();
		while ((ent = readdir (dir)) != nullptr) {
#ifdef DIRENT_HAS_TYPE_FIELD
			if(ent->d_type == DT_REG) {
#endif
				std::string fn = ent->d_name;
				if(!shv::core::String::endsWith(fn, ext))
					continue;
				try {
					int64_t msec = fileNameToFileMsec(fn);
					m_journalStatus.files.push_back(msec);
					fn = m_journalDir + '/' + fn;
					int64_t sz = file_size(fn);
					if(msec > max_file_msec) {
						max_file_msec = msec;
						m_journalStatus.lastFileSize = sz;
					}
					m_journalStatus.journalSize += sz;
				} catch (std::logic_error &e) {
					shvWarning() << "Mallformated shv journal file name" << fn << e.what();
				}
#ifdef DIRENT_HAS_TYPE_FIELD
			}
#endif
		}
		closedir (dir);
		std::sort(m_journalStatus.files.begin(), m_journalStatus.files.end());
		logDShvJournal() << "journal dir contains:" << m_journalStatus.files.size() << "files";
		if(m_journalStatus.files.size()) {
			logDShvJournal() << "first file:"
							 << m_journalStatus.files[0]
							 << cp::RpcValue::DateTime::fromMSecsSinceEpoch(m_journalStatus.files[0]).toIsoString();
			logDShvJournal() << "last file:"
							 << m_journalStatus.files[m_journalStatus.files.size()-1]
							 << cp::RpcValue::DateTime::fromMSecsSinceEpoch(m_journalStatus.files[m_journalStatus.files.size()-1]).toIsoString();
		}
	}
	else {
		throw std::runtime_error("Cannot read content of dir: " + m_journalDir);
	}
}

void FileShvJournal2::updateRecentTimeStamp()
{
	if(m_journalStatus.files.empty()) {
		m_journalStatus.recentTimeStamp = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	}
	else {
		std::string fn = fileMsecToFilePath(m_journalStatus.files[m_journalStatus.files.size() - 1]);
		m_journalStatus.recentTimeStamp = findLastEntryDateTime(fn);
		if(m_journalStatus.recentTimeStamp < 0) {
			// corrupted file, start new one
			m_journalStatus.recentTimeStamp = cp::RpcValue::DateTime::now().msecsSinceEpoch();
		}
	}
	logDShvJournal() << "update recent time stamp:" << m_journalStatus.recentTimeStamp << cp::RpcValue::DateTime::fromMSecsSinceEpoch(m_journalStatus.recentTimeStamp).toIsoString();
}

int64_t FileShvJournal2::findLastEntryDateTime(const std::string &fn)
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
						size_t len;
						chainpack::RpcValue::DateTime dt = chainpack::RpcValue::DateTime::fromUtcString(s, &len);
						if(len > 0)
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

chainpack::RpcValue FileShvJournal2::getLog(const ShvJournalGetLogParams &params)
{
	try {
		return getLogThrow(params);
	}
	catch (std::exception &e) {
		logIShvJournal() << "Get log failed, journal dir will be read again, SD card might be replaced, error:" << e.what();
	}
	checkJournalConsistecy(true);
	return getLogThrow(params);
}

chainpack::RpcValue FileShvJournal2::getLogThrow(const ShvJournalGetLogParams &params)
{
	logIShvJournal() << "========================= getLog ==================";
	logIShvJournal() << "params:" << params.toRpcValue().toCpon();
	checkJournalConsistecy();
	cp::RpcValue::List log;
	cp::RpcValue::Map path_cache;
	int rec_cnt = 0;
	auto since_msec = params.since.isDateTime()? params.since.toDateTime().msecsSinceEpoch(): 0;
	auto until_msec = params.until.isDateTime()? params.until.toDateTime().msecsSinceEpoch(): 0;
	if(m_journalStatus.files.size()) {
		std::vector<int64_t>::const_iterator file_it = m_journalStatus.files.begin();
		if(since_msec > 0) {
			logDShvJournal() << "since:" << params.since.toCpon() << "msec:" << since_msec;
			file_it = std::lower_bound(m_journalStatus.files.begin(), m_journalStatus.files.end(), since_msec);
			if(file_it == m_journalStatus.files.end()) {
				/// take last file
				--file_it;
				logDShvJournal() << "\t" << "not found, taking last file:" << *file_it << fileMsecToFileName(*file_it);
			}
			else if(*file_it == since_msec) {
				/// take exactly this file
				logDShvJournal() << "\t" << "found exactly:" << *file_it << fileMsecToFileName(*file_it);
			}
			else if(file_it == m_journalStatus.files.begin()) {
				/// take first file
				logDShvJournal() << "\t" << "begin, taking first file:" << *file_it << fileMsecToFileName(*file_it);
			}
			else {
				/// take previous file
				logDShvJournal() << "\t" << "lower bound found, taking previous file:" << *file_it << fileMsecToFileName(*file_it);
				--file_it;
			}
		}
		/// this ensure that there be only one copy of each path in memory
		int max_path_id = 0;
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
		int max_rec_cnt = std::min(params.maxRecordCount, m_getLogRecordCountLimit);
		struct SnapshotEntry
		{
			std::string uptime;
			std::string value;
			std::string shorttime;
			std::string domain;
		};
		std::map<std::string, SnapshotEntry> snapshot;

		const std::string fnames[] = {"foo.txt", "bar.txt", "baz.dat", "zoidberg"};
		std::regex domain_regex;
		if(!params.domainPattern.empty())
			domain_regex = std::regex{params.domainPattern};
		for(; file_it != m_journalStatus.files.end(); file_it++) {
			std::string fn = fileMsecToFilePath(*file_it);
			logDShvJournal() << "-------- opening file:" << fn;
			if(!file_exists(fn))
				throw std::runtime_error("File: " + fn + " doesn't exist, journal state is corrupted.");
			std::ifstream in(fn, std::ios::in | std::ios::binary);
			if(!in) {
				logWShvJournal() << "Cannot open file:" << fn << "for reading.";
				continue;
			}
			while(in) {
				std::string line = getLine(in, RECORD_SEPARATOR);
				//logDShvJournal() << "line:" << line;
				shv::core::StringView sv(line);
				shv::core::StringViewList line_record = sv.split(FIELD_SEPARATOR, shv::core::StringView::KeepEmptyParts);
				if(line_record.empty()) {
					logDShvJournal() << "\t skipping empty line";
					continue; // skip empty line
				}
				std::string dtstr = line_record[Column::Timestamp].toString();
				ShvPath path = line_record.value(Column::Path).toString();
				if(!params.pathPattern.empty() && !path.matchWild(params.pathPattern))
					continue;
				std::string domain_str = line_record.value(Column::Domain).toString();
				if(!params.domainPattern.empty()) {
					if(!std::regex_match(domain_str, domain_regex))
							continue;
				}
				size_t len;
				cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromUtcString(dtstr, &len);
				if(len == 0) {
					logWShvJournal() << fn << "invalid date time string:" << dtstr;
					continue;
				}
				StringView short_time_sv = line_record.value(Column::ShortTime);
				//logDShvJournal() << "\t FIELDS:" << dtstr << '\t' << path << "vals:" << lst.join('|');
				if(since_msec > 0 && dt.msecsSinceEpoch() < since_msec) {
					if(params.withSnapshot)
						snapshot[path] = SnapshotEntry{
								line_record.value(Column::UpTime).toString(),
								line_record.value(Column::Value).toString(),
								short_time_sv.toString(),
								domain_str
				};
				}
				else {
					if(params.withSnapshot && !snapshot.empty()) {
						logDShvJournal() << "\t -------------- Snapshot";
						std::string err;
						for(const auto &kv : snapshot) {
							cp::RpcValue::List rec;
							rec.push_back(params.since);
							if(params.withUptime)
								rec.push_back(cp::RpcValue::fromCpon(kv.second.uptime, &err));
							rec.push_back(make_path_shared(kv.first));
							rec.push_back(cp::RpcValue::fromCpon(kv.second.value, &err));
							rec.push_back(cp::RpcValue::fromCpon(kv.second.shorttime, &err));
							rec.push_back(kv.second.domain.empty()? cp::RpcValue(nullptr): kv.second.domain);
							log.push_back(rec);
							rec_cnt++;
							if(rec_cnt >= max_rec_cnt)
								goto log_finish;
						}
						snapshot.clear();
					}
					if(until_msec == 0 || dt.msecsSinceEpoch() <= until_msec) {
						std::string err;
						cp::RpcValue::List rec;
						rec.push_back(dt);
						if(params.withUptime)
							rec.push_back(cp::RpcValue::fromCpon(line_record.value(Column::UpTime).toString(), &err));
						rec.push_back(make_path_shared(path));
						rec.push_back(cp::RpcValue::fromCpon(line_record.value(Column::Value).toString(), &err));
						rec.push_back(cp::RpcValue::fromCpon(short_time_sv.toString(), &err));
						rec.push_back(domain_str.empty()? cp::RpcValue(nullptr): domain_str);
						log.push_back(rec);
						rec_cnt++;
						if(rec_cnt >= max_rec_cnt)
							goto log_finish;
					}
					else {
						goto log_finish;
					}
				}
			}
		}
	}
log_finish:
	cp::RpcValue ret = log;
	cp::RpcValue::MetaData md;
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::BasicInfo)) {
		{
			cp::RpcValue::Map device;
			device["id"] = m_deviceId;
			device["type"] = m_deviceType;
			md.setValue("device", device); // required
		}
		md.setValue("logVersion", 1); // required
		md.setValue("dateTime", cp::RpcValue::DateTime::now());
		ShvJournalGetLogParams params2 = params;
		params2.since = (since_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(since_msec)): cp::RpcValue(nullptr);
		params2.until = (until_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(until_msec)): cp::RpcValue(nullptr);
		md.setValue("params", params2.toRpcValue());
		md.setValue(KEY_RECORD_COUNT, rec_cnt);
	}
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::FieldInfo)) {
		cp::RpcValue::List fields;
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Timestamp)}});
		if(params.withUptime)
			fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::UpTime)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Path)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Value)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::ShortTime)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Domain)}});
		md.setValue("fields", std::move(fields));
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
		md.setValue(KEY_PATHS_DICT, path_dict);
	}
	if(!md.isEmpty())
		ret.setMetaData(std::move(md));
	return ret;
}

std::string FileShvJournal2::getLine(std::istream &in, char sep)
{
	std::string line;
	while(in) {
		char buff[1024];
		in.getline(buff, sizeof (buff), sep);
		std::string s(buff);
		line += s;
		if(in.gcount() == (long)s.size()) {
			// separator not found
			continue;
		}
		break;
	}
	return line;
}

const char *FileShvJournal2::Column::name(FileShvJournal2::Column::Enum e)
{
	switch (e) {
	case Column::Enum::Timestamp: return "timestamp";
	case Column::Enum::UpTime: return "upTime";
	case Column::Enum::Path: return "path";
	case Column::Enum::Value: return "value";
	case Column::Enum::ShortTime: return "shortTime";
	case Column::Enum::Domain: return "domain";
	}
	return "invalid";
}

} // namespace utils
} // namespace core
} // namespace shv
