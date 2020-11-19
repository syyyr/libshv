#include "shvfilejournal.h"
#include "shvjournalfilewriter.h"
#include "shvjournalfilereader.h"
#include "shvlogheader.h"
#include "shvpath.h"

#include "../log.h"
#include "../exception.h"
#include "../string.h"
#include "../stringview.h"

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
#define logMShvJournal() shvCMessage("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

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
#if defined(__unix) || defined(__APPLE__)
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
	shvWarning() << dir_name << "exists:" << is_dir(dir_name);
	if(is_dir(dir_name))
		return true;
	logIShvJournal() << "Creating journal dir:" << dir_name;
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
/*
static bool file_exists(const std::string &file_name)
{
	SHV_STATBUF st;
	return (SHV_STAT(file_name.data(), &st) == 0);
}
*/
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
// FileShvJournal2
//==============================================================
static constexpr size_t MIN_SEP_POS = 13;
static constexpr size_t SEC_SEP_POS = MIN_SEP_POS + 3;
static constexpr size_t MSEC_SEP_POS = SEC_SEP_POS + 3;

const std::string ShvFileJournal::FILE_EXT = ".log2";

ShvFileJournal::ShvFileJournal(std::string device_id, ShvFileJournal::SnapShotFn snf)
	: m_snapShotFn(snf)
	//, m_appendLogTSNowFn([]() {return cp::RpcValue::DateTime::now().msecsSinceEpoch();})
{
	//setDefaultAppendLogTSNowFn();
	setDeviceId(device_id);
}

//void ShvFileJournal::setDefaultAppendLogTSNowFn()
//{
//	m_appendLogTSNowFn = []() {return cp::RpcValue::DateTime::now().msecsSinceEpoch();};
//}

void ShvFileJournal::setJournalDir(std::string s)
{
	if(s == m_journalContext.journalDir)
		return;
	shvInfo() << "Journal dir set to:" << s;
	m_journalContext.journalDir = std::move(s);
}

const std::string &ShvFileJournal::journalDir()
{
	if(m_journalContext.journalDir.empty()) {
		std::string d = "/tmp/shvjournal/";
		if(m_journalContext.deviceId.empty()) {
			d += "default";
		}
		else {
			std::string id = m_journalContext.deviceId;
			String::replace(id, '/', '-');
			String::replace(id, ':', '-');
			String::replace(id, '.', '-');
			d += id;
		}
		m_journalContext.journalDir = d;
		shvInfo() << "Journal dir not set, falling back to default value:" << journalDir();
	}
	return m_journalContext.journalDir;
}

void ShvFileJournal::setFileSizeLimit(const std::string &n)
{
	setFileSizeLimit(str_to_size(n));
}

void ShvFileJournal::setJournalSizeLimit(const std::string &n)
{
	setJournalSizeLimit(str_to_size(n));
}

void ShvFileJournal::append(const ShvJournalEntry &entry)
{
	try {
		appendThrow(entry);
		return;
	}
	catch (std::exception &e) {
		logIShvJournal() << "Append to log failed, journal dir will be read again, SD card might be replaced:" << e.what();
	}
	try {
		ensureJournalDir();
		checkJournalContext_helper(true);
		appendThrow(entry);
	}
	catch (std::exception &e) {
		logWShvJournal() << "Append to log failed after journal dir check:" << e.what();
	}
}

void ShvFileJournal::appendThrow(const ShvJournalEntry &entry)
{
	shvLogFuncFrame();// << "last file no:" << lastFileNo();

	checkJournalContext_helper();
	checkRecentTimeStamp();

	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch(); //m_appendLogTSNowFn();
	if(msec < m_journalContext.recentTimeStamp)
		msec = m_journalContext.recentTimeStamp;
	int64_t journal_file_start_msec = 0;
	if(m_journalContext.files.empty()) {
		journal_file_start_msec = msec;
	}
	else if(m_journalContext.lastFileSize > m_fileSizeLimit) {
		/// create new file
		journal_file_start_msec = msec;
	}
	else {
		journal_file_start_msec = m_journalContext.files[m_journalContext.files.size() - 1];
	}
	if(!m_journalContext.files.empty() && journal_file_start_msec < m_journalContext.files[m_journalContext.files.size() - 1])
		SHV_EXCEPTION("Journal context corrupted!");

	ShvJournalFileWriter wr(journalDir(), journal_file_start_msec, m_journalContext.recentTimeStamp);
	ssize_t orig_fsz = wr.fileSize();
	if(orig_fsz == 0) {
		logMShvJournal() << "New log file:" << wr.fileName() << "created.";
		// new file should start with snapshot
		if(m_snapShotFn) {
			std::vector<ShvJournalEntry> snapshot;
			m_snapShotFn(snapshot);
			logDShvJournal() << "Writing snapshot, entries count:" << snapshot.size();
			for(ShvJournalEntry &e : snapshot) {
				e.epochMsec = journal_file_start_msec;
				wr.appendMonotonic(e);
			}
		}
		else {
			logMShvJournal() << "SnapShot function not defined";
		}
		m_journalContext.files.push_back(journal_file_start_msec);
	}
	wr.appendMonotonic(entry);
	m_journalContext.recentTimeStamp = wr.recentTimeStamp();
	ssize_t new_fsz = wr.fileSize();
	m_journalContext.lastFileSize = new_fsz;
	m_journalContext.journalSize += new_fsz - orig_fsz;
	if(m_journalContext.journalSize > m_journalSizeLimit) {
		rotateJournal();
	}
}

int64_t ShvFileJournal::JournalContext::fileNameToFileMsec(const std::string &fn)
{
	std::string utc_str;
	auto ix = fn.rfind('/');
	if(ix != std::string::npos)
		utc_str = fn.substr(ix + 1);
	else
		utc_str = fn;
	if(MSEC_SEP_POS >= utc_str.size())
		SHV_EXCEPTION("fileNameToFileMsec(): File name: '" + fn + "' too short.");
	utc_str[MIN_SEP_POS] = ':';
	utc_str[SEC_SEP_POS] = ':';
	utc_str[MSEC_SEP_POS] = '.';
	size_t len;
	int64_t msec = cp::RpcValue::DateTime::fromUtcString(utc_str, &len).msecsSinceEpoch();
	if(msec == 0 || len == 0)
		SHV_EXCEPTION("fileNameToFileMsec(): Invalid file name: '" + fn + "' cannot be converted to date time");
	return msec;
}

std::string ShvFileJournal::JournalContext::msecToBaseFileName(int64_t msec)
{
	std::string fn = cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString(cp::RpcValue::DateTime::MsecPolicy::Always, false);
	fn[MIN_SEP_POS] = '-';
	fn[SEC_SEP_POS] = '-';
	fn[MSEC_SEP_POS] = '-';
	return fn;
}

std::string ShvFileJournal::JournalContext::fileMsecToFileName(int64_t msec)
{
	return msecToBaseFileName(msec) + FILE_EXT;
}

std::string ShvFileJournal::JournalContext::fileMsecToFilePath(int64_t file_msec) const
{
	std::string fn = fileMsecToFileName(file_msec);
	return journalDir + '/' + fn;
}

void ShvFileJournal::checkJournalContext_helper(bool force)
{
	if(!m_journalContext.isConsistent() || force) {
		logMShvJournal() << "journal status not consistent or check forced";
		m_journalContext.recentTimeStamp = 0;
		m_journalContext.journalDirExists = journalDirExists();
		if(!m_journalContext.journalDirExists)
			ensureJournalDir();
		if(m_journalContext.journalDirExists)
			updateJournalStatus();
		else
			shvWarning() << "Journal dir:" << journalDir() << "does not exist!";
	}
	if(!m_journalContext.isConsistent()) {
		SHV_EXCEPTION("Journal cannot be brought to consistent state.");
	}
}

void ShvFileJournal::ensureJournalDir()
{
	if(!mkpath(journalDir())) {
		m_journalContext.journalDirExists = false;
		SHV_EXCEPTION("Journal dir: " + m_journalContext.journalDir + " do not exists and cannot be created");
	}
	//logDShvJournal() << "journal dir:" << m_journalStatus.journalDir << "exists";
	m_journalContext.journalDirExists = true;
}

bool ShvFileJournal::journalDirExists()
{
	return is_dir(journalDir());
}

void ShvFileJournal::rotateJournal()
{
	logMShvJournal() << "Rotating journal of size:" << m_journalContext.journalSize;
	updateJournalFiles();
	size_t file_sz = m_journalContext.files.size();
	size_t file_cnt = m_journalContext.files.size();
	for(int64_t file_msec : m_journalContext.files) {
		if(file_cnt == 1) {
			/// keep at least one file in case of bad limits configuration
			break;
		}
		if(m_journalContext.journalSize < m_journalSizeLimit)
			break;
		std::string fn = m_journalContext.fileMsecToFilePath(file_msec);
		logMShvJournal() << "\t deleting file:" << fn;
		m_journalContext.journalSize -= rm_file(fn);
		file_sz--;
		file_cnt--;
	}
	updateJournalStatus();
	logMShvJournal() << "New journal of size:" << m_journalContext.journalSize;
}

void ShvFileJournal::convertLog1JournalDir()
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
								std::string new_fn = journal_dir + '/' + m_journalContext.fileMsecToFileName(file_msec);
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
void ShvFileJournal::updateJournalStatus()
{
	updateJournalFiles();
}

void ShvFileJournal::updateJournalFiles()
{
	logMShvJournal() << "FileShvJournal2::updateJournalFiles()";
	m_journalContext.journalSize = 0;
	m_journalContext.lastFileSize = 0;
	m_journalContext.files.clear();
	int64_t max_file_msec = -1;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (m_journalContext.journalDir.c_str())) != nullptr) {
		m_journalContext.journalSize = 0;
		const std::string &ext = FILE_EXT;
		while ((ent = readdir (dir)) != nullptr) {
#ifdef DIRENT_HAS_TYPE_FIELD
			if(ent->d_type == DT_REG) {
#endif
				std::string fn = ent->d_name;
				if(!shv::core::String::endsWith(fn, ext))
					continue;
				try {
					int64_t msec = m_journalContext.fileNameToFileMsec(fn);
					m_journalContext.files.push_back(msec);
					fn = m_journalContext.journalDir + '/' + fn;
					int64_t sz = file_size(fn);
					if(msec > max_file_msec) {
						max_file_msec = msec;
						m_journalContext.lastFileSize = sz;
					}
					m_journalContext.journalSize += sz;
				} catch (std::logic_error &e) {
					shvWarning() << "Mallformated shv journal file name" << fn << e.what();
				}
#ifdef DIRENT_HAS_TYPE_FIELD
			}
#endif
		}
		closedir (dir);
		std::sort(m_journalContext.files.begin(), m_journalContext.files.end());
		logMShvJournal() << "journal dir contains:" << m_journalContext.files.size() << "files";
		if(m_journalContext.files.size()) {
			logMShvJournal() << "first file:"
							 << m_journalContext.files[0]
							 << cp::RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.files[0]).toIsoString();
			logMShvJournal() << "last file:"
							 << m_journalContext.files[m_journalContext.files.size()-1]
							 << cp::RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.files[m_journalContext.files.size()-1]).toIsoString();
		}
	}
	else {
		SHV_EXCEPTION("Cannot read content of dir: " + m_journalContext.journalDir);
	}
}
/*
int64_t ShvFileJournal::lastEntryTimeStamp()
{
	if(m_journalContext.files.empty())
		return 0;
	std::string fn = m_journalContext.fileMsecToFilePath(m_journalContext.files[m_journalContext.files.size() - 1]);
	return findLastEntryDateTime(fn);
}
*/
void ShvFileJournal::checkRecentTimeStamp()
{
	if(m_journalContext.recentTimeStamp > 0)
		return;
	logDShvJournal() << "FileShvJournal2::updateRecentTimeStamp()";
	if(m_journalContext.files.empty()) {
		// recentTimeStamp will be set on first appendLog() call
		logMShvJournal() << "journal dir is empty, setting recent timestamp to 0";
		m_journalContext.recentTimeStamp = 0; //cp::RpcValue::DateTime::now().msecsSinceEpoch();
	}
	else {
		std::string fn = m_journalContext.fileMsecToFilePath(m_journalContext.files[m_journalContext.files.size() - 1]);
		m_journalContext.recentTimeStamp = findLastEntryDateTime(fn);
		logMShvJournal() << "setting recent timestamp to last entry in:" << fn
						 << "to:" << m_journalContext.recentTimeStamp << "epoch msec"
						 << cp::RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.recentTimeStamp).toIsoString();
		if(m_journalContext.recentTimeStamp < 0) {
			logWShvJournal() << "corrupted log file:" << fn;
			m_journalContext.recentTimeStamp = 0;
		}
	}
	//logDShvJournal() << "update recent time stamp:" << m_journalContext.recentTimeStamp << cp::RpcValue::DateTime::fromMSecsSinceEpoch(m_journalContext.recentTimeStamp).toIsoString();
}

int64_t ShvFileJournal::findLastEntryDateTime(const std::string &fn, ssize_t *p_date_time_fpos)
{
	shvLogFuncFrame() << "'" + fn + "'";
	ssize_t date_time_fpos = -1;
	if(p_date_time_fpos)
		*p_date_time_fpos = date_time_fpos;
	std::ifstream in(fn, std::ios::in | std::ios::binary);
	if (!in)
		SHV_EXCEPTION("Cannot open file: " + fn + " for reading.");
	int64_t dt_msec = -1;
	in.seekg(0, std::ios::end);
	long fpos = in.tellg();
	static constexpr int TS_LEN = 30;
	static constexpr int CHUNK_LEN = 512;
	char buff[CHUNK_LEN + TS_LEN];
	logDShvJournal() << "------------------findLastEntryDateTime-----------------------------" << fn;
	while(fpos > 0) {
		in.seekg(-CHUNK_LEN, std::ios::cur);
		fpos = in.tellg();
		if(fpos < 0) {
			in.clear();
			in.seekg(0);
			fpos = 0;
		}
		// date time string can be partialy on end of this chunk and at beggining of next,
		// read little bit more data to cover this
		// serialized date-time should never exceed 28 bytes see: 2018-01-10T12:03:56.123+0130
		in.read(buff, sizeof (buff));
		auto n = in.gcount();
		if(in.eof()) {
			in.clear();
			in.seekg(-n, std::ios::end);
		}
		else {
			in.seekg(-n, std::ios::cur);
		}
		//shvInfo() << "fpos:" << fpos << "n:" << n << in.tellg();
		if(n == 0)
			break;

		std::string chunk(buff, static_cast<size_t>(n));
		logDShvJournal() << "fpos:" << fpos << "chunk:" << chunk;
		size_t line_start_pos = 0;
		if(fpos == 0) {
			line_start_pos = 0;
		}
		else {
			auto pos = chunk.length() - 1;
			// remove trailing blanks, like trailing '\n' in log file
			for(; pos > 0; --pos)
				if(!(chunk[pos] == '\n' || chunk[pos] == '\t' || chunk[pos] == ' '))
					break;
			line_start_pos = chunk.rfind(RECORD_SEPARATOR, pos);
			if(line_start_pos != std::string::npos)
				line_start_pos++;
		}
		if(line_start_pos != std::string::npos) {
			auto tab_pos = chunk.find(FIELD_SEPARATOR, line_start_pos);
			if(tab_pos != std::string::npos) {
				std::string s = chunk.substr(line_start_pos, tab_pos - line_start_pos);
				logDShvJournal() << "\t checking:" << s;
				size_t len;
				chainpack::RpcValue::DateTime dt = chainpack::RpcValue::DateTime::fromUtcString(s, &len);
				if(len > 0) {
					date_time_fpos = fpos + (ssize_t)line_start_pos;
					dt_msec = dt.msecsSinceEpoch();
				}
				else {
					logWShvJournal() << fn << "Malformed shv journal date time:" << s << "will be ignored.";
				}
			}
			else if(chunk.size() > line_start_pos) {
				logWShvJournal() << fn << "Truncated shv journal date time:" << chunk.substr(line_start_pos) << "will be ignored.";
			}
		}

		if(dt_msec > 0) {
			logDShvJournal() << "\t return:" << dt_msec << chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(dt_msec).toIsoString();
			if(p_date_time_fpos)
				*p_date_time_fpos = date_time_fpos;
			return dt_msec;
		}
	}
	logWShvJournal() << fn << "File does not contain record with valid date time";
	return -1;
}

const ShvFileJournal::JournalContext &ShvFileJournal::checkJournalContext()
{
	try {
		checkJournalContext_helper();
	}
	catch (std::exception &e) {
		logIShvJournal() << "Check journal consistecy failed, journal dir will be read again, SD card might be replaced, error:" << e.what();
	}
	checkJournalContext_helper(true);
	return m_journalContext;
}

chainpack::RpcValue ShvFileJournal::getLog(const ShvGetLogParams &params)
{
	JournalContext ctx = checkJournalContext();
	return getLog(ctx, params);
}

chainpack::RpcValue ShvFileJournal::getSnapShotMap()
{
	if(!m_snapShotFn)
		SHV_EXCEPTION("SnapShot function not defined");
	std::vector<ShvJournalEntry> snapshot;
	m_snapShotFn(snapshot);
	cp::RpcValue::Map m;
	for(const ShvJournalEntry &e : snapshot) {
		if(e.value.isValid())
			m[e.path] = e.value;
	}
	return m;
}

chainpack::RpcValue ShvFileJournal::getLog(const ShvFileJournal::JournalContext &journal_context, const ShvGetLogParams &params)
{
	logIShvJournal() << "========================= getLog ==================";
	logIShvJournal() << "params:" << params.toRpcValue().toCpon();
	std::map<std::string, ShvJournalEntry> snapshot;
	cp::RpcValue::List log;

	cp::RpcValue::Map path_cache;
	const auto params_since_msec = params.since.isDateTime()? params.since.toDateTime().msecsSinceEpoch(): 0;
	const auto params_until_msec = params.until.isDateTime()? params.until.toDateTime().msecsSinceEpoch(): 0;
	int64_t journal_start_msec = 0;
	int64_t first_record_msec = 0;
	int64_t last_record_msec = 0;
	int rec_cnt_limit = std::min(params.recordCountLimit, DEFAULT_GET_LOG_RECORD_COUNT_LIMIT);
	bool rec_cnt_limit_hit = false;

	/// this ensure that there be only one copy of each path in memory
	int max_path_id = 0;
	auto make_path_shared = [&path_cache, &max_path_id, &params](const std::string &path) -> cp::RpcValue {
		cp::RpcValue ret = path_cache.value(path);
		if(ret.isValid())
			return ret;
		if(params.withPathsDict)
			ret = ++max_path_id;
		else
			ret = path;
		logMShvJournal() << "Adding record to path cache:" << path << "-->" << ret.toCpon();
		path_cache[path] = ret;
		return ret;
	};
	auto append_log_entry = [make_path_shared, rec_cnt_limit, &rec_cnt_limit_hit, &first_record_msec, &last_record_msec, &log](const ShvJournalEntry &e) {
		if((int)log.size() >= rec_cnt_limit) {
			rec_cnt_limit_hit = true;
			return false;
		}
		if(first_record_msec == 0)
			first_record_msec = e.epochMsec;
		last_record_msec = e.epochMsec;
		cp::RpcValue::List rec;
		rec.push_back(e.dateTime());
		rec.push_back(make_path_shared(e.path));
		rec.push_back(e.value);
		rec.push_back(e.shortTime == ShvJournalEntry::NO_SHORT_TIME? cp::RpcValue(nullptr): cp::RpcValue(e.shortTime));
		rec.push_back((e.domain.empty() || e.domain == cp::Rpc::SIG_VAL_CHANGED)? cp::RpcValue(nullptr): e.domain);
		rec.push_back((int)e.sampleType);
		rec.push_back(e.userId.empty()? cp::RpcValue(nullptr): cp::RpcValue(e.userId));
		log.push_back(std::move(rec));
		return true;
	};
	auto write_snapshot = [append_log_entry, &snapshot]() {
		if(!snapshot.empty()) {
			logDShvJournal() << "\t -------------- Snapshot";
			for(const auto &kv : snapshot) {
				const ShvJournalEntry &e = kv.second;
				if(!append_log_entry(e))
					return false;
			}
			snapshot.clear();
		}
		return true;
	};
	/*
	auto append_data_missing = [&append_log_entry](int64_t msec, bool b) {
		ShvJournalEntry e;
		e.epochMsec = msec;
		e.path = ShvJournalEntry::PATH_DATA_MISSING;
		e.value = b? ShvJournalEntry::DATA_MISSING_LOG_FILE_MISSING: "";
		e.domain = ShvJournalEntry::DOMAIN_SHV_SYSTEM;
		return append_log_entry(e);
	};
	*/
	if(journal_context.files.size()) {
		std::vector<int64_t>::const_iterator file_it = journal_context.files.begin();
		journal_start_msec = *file_it;
		if(params_since_msec > 0) {
			logDShvJournal() << "since:" << params.since.toCpon() << "msec:" << params_since_msec;
			file_it = std::lower_bound(journal_context.files.begin(), journal_context.files.end(), params_since_msec);
			if(file_it == journal_context.files.end()) {
				/// take last file
				--file_it;
				logDShvJournal() << "\t" << "not found, taking last file:" << *file_it << journal_context.fileMsecToFileName(*file_it);
			}
			else if(*file_it == params_since_msec) {
				/// take exactly this file
				logDShvJournal() << "\t" << "found exactly:" << *file_it << journal_context.fileMsecToFileName(*file_it);
			}
			else if(file_it == journal_context.files.begin()) {
				/// take first file
				logDShvJournal() << "\t" << "begin, taking first file:" << *file_it << journal_context.fileMsecToFileName(*file_it);
			}
			else {
				/// take previous file
				logDShvJournal() << "\t" << "lower bound found, taking previous file:" << *file_it << journal_context.fileMsecToFileName(*file_it);
				--file_it;
			}
		}

		//if(log_since_msec > 0 && log_since_msec < journal_start_msec) {
		//	append_data_missing(log_since_msec, true);
		//	append_data_missing(journal_start_msec, false);
		//}

		PatternMatcher pattern_matcher(params);
		for(; file_it != journal_context.files.end(); file_it++) {
			std::string fn = journal_context.fileMsecToFilePath(*file_it);
			logDShvJournal() << "-------- opening file:" << fn;
			ShvJournalFileReader rd(fn);
			while(rd.next()) {
				const ShvJournalEntry &e = rd.entry();
				if(!params.pathPattern.empty()) {
					logDShvJournal() << "\t MATCHING:" << params.pathPattern << "vs:" << e.path;
					if(!pattern_matcher.match(e.path, e.domain))
						continue;
					logDShvJournal() << "\t\t MATCH";
				}
				if(params_since_msec > 0 && e.epochMsec < params_since_msec) {
					if(params.withSnapshot) {
						if(e.sampleType == ShvJournalEntry::SampleType::Continuous) {
							ShvJournalEntry e2 = e;
							e2.epochMsec = params_since_msec;
							snapshot[e2.path] = std::move(e2);
						}
					}
				}
				else {
					if(params.withSnapshot)
						if(!write_snapshot())
							goto log_finish;
					if(params_until_msec == 0 || e.epochMsec < params_until_msec) { // keep interval open to make log merge simpler
						if(!append_log_entry(e))
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
	if(params.withSnapshot) {
		// snapshot should be written already
		// this is only case, when log is empty and
		// only snapshot shall be returned
		write_snapshot();
	}

	int64_t log_since_msec = params_since_msec;
	if(log_since_msec < journal_start_msec) {
		log_since_msec = journal_start_msec;
	}
	int64_t log_until_msec = params_until_msec;
	if(params_until_msec == 0 || rec_cnt_limit_hit) {
		log_until_msec = last_record_msec;
	}
	cp::RpcValue ret = log;
	ShvLogHeader log_header;
	{
		log_header.setDeviceId(journal_context.deviceId);
		log_header.setDeviceType(journal_context.deviceType);
		log_header.setDateTime(cp::RpcValue::DateTime::now());
		log_header.setLogParams(params);
		log_header.setSince((log_since_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(log_since_msec)): cp::RpcValue(nullptr));
		log_header.setUntil((log_until_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(log_until_msec)): cp::RpcValue(nullptr));
		log_header.setRecordCount((int)log.size());
		log_header.setRecordCountLimit(rec_cnt_limit);
		log_header.setRecordCountLimitHit(rec_cnt_limit_hit);
		log_header.setWithSnapShot(params.withSnapshot);
		log_header.setWithPathsDict(params.withPathsDict);

		using Column = ShvLogHeader::Column;
		cp::RpcValue::List fields;
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Timestamp)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Path)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Value)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::ShortTime)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Domain)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::SampleType)}});
		log_header.setFields(std::move(fields));
	}
	if(params.withPathsDict) {
		logIShvJournal() << "Generating paths dict";
		cp::RpcValue::IMap path_dict;
		for(auto kv : path_cache) {
			//logIShvJournal() << "Adding record to paths dict:" << kv.second.toInt() << "-->" << kv.first;
			path_dict[kv.second.toInt()] = kv.first;
		}
		log_header.setPathDict(std::move(path_dict));
	}
	if(params.withTypeInfo) {
		log_header.setTypeInfo(journal_context.typeInfo);
	}
	ret.setMetaData(log_header.toMetaData());
	return ret;
}

const char *ShvFileJournal::TxtColumn::name(ShvFileJournal::TxtColumn::Enum e)
{
	switch (e) {
	case TxtColumn::Enum::Timestamp: return ShvLogHeader::Column::name(ShvLogHeader::Column::Timestamp);
	case TxtColumn::Enum::UpTime: return "upTime";
	case TxtColumn::Enum::Path: return ShvLogHeader::Column::name(ShvLogHeader::Column::Path);
	case TxtColumn::Enum::Value: return ShvLogHeader::Column::name(ShvLogHeader::Column::Value);
	case TxtColumn::Enum::ShortTime: return ShvLogHeader::Column::name(ShvLogHeader::Column::ShortTime);
	case TxtColumn::Enum::Domain: return ShvLogHeader::Column::name(ShvLogHeader::Column::Domain);
	case TxtColumn::Enum::SampleType: return ShvLogHeader::Column::name(ShvLogHeader::Column::SampleType);
	case TxtColumn::Enum::UserId: return ShvLogHeader::Column::name(ShvLogHeader::Column::UserId);
	}
	return "invalid";
}

} // namespace utils
} // namespace core
} // namespace shv
