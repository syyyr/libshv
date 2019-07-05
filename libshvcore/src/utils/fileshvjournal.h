#ifndef SHV_CORE_UTILS_FILESHVJOURNAL_H
#define SHV_CORE_UTILS_FILESHVJOURNAL_H

#include "../shvcoreglobal.h"
#include "shvjournalgetlogparams.h"
#include "../utils.h"

#include <shv/chainpack/rpcvalue.h>

#include <functional>
#include <vector>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvJournalEntry
{
public:
	static const char *DOMAIN_VAL_CHANGE; /// see shv::chainpack::Rpc::SIG_VAL_CHANGED
	static const char *DOMAIN_VAL_FASTCHANGE; /// see shv::chainpack::Rpc::SIG_VAL_FASTCHANGED
	static const char *DOMAIN_VAL_SERVICECHANGE; /// see shv::chainpack::Rpc::SIG_SERVICE_VAL_CHANGED

	static constexpr int NO_SHORT_TIME = -1;

	std::string path;
	shv::chainpack::RpcValue value;
	int shortTime = NO_SHORT_TIME;
	std::string domain;

	ShvJournalEntry() {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, std::string domain, int short_time)
		: path(std::move(path))
		, value{value}
		, shortTime(short_time)
		, domain(std::move(domain))
	{
	}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value)
		: ShvJournalEntry(path, value, DOMAIN_VAL_CHANGE, NO_SHORT_TIME) {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, int short_time)
		: ShvJournalEntry(path, value, DOMAIN_VAL_FASTCHANGE, short_time) {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, std::string domain)
		: ShvJournalEntry(path, value, std::move(domain), NO_SHORT_TIME) {}

	bool isValid() const {return !path.empty() && value.isValid();}
	void setShortTime(int short_time) {shortTime = short_time;}
};

class SHVCORE_DECL_EXPORT FileShvJournal
{
public:
	static constexpr long DEFAULT_FILE_SIZE_LIMIT = 100 * 1024;
	static constexpr long DEFAULT_JOURNAL_SIZE_LIMIT = 100 * 100 * 1024;
	static constexpr int DEFAULT_GET_LOG_RECORD_COUNT_LIMIT = 100 * 1000;
	static constexpr int FILE_DIGITS = 6;
	static const char* FILE_EXT;
	static constexpr char FIELD_SEPARATOR = '\t';
	static constexpr char RECORD_SEPARATOR = '\n';

	static const char* KEY_NAME;
	static const char *KEY_RECORD_COUNT;
	static const char *KEY_PATHS_DICT;

	struct SHVCORE_DECL_EXPORT Column
	{
		enum Enum {
			Timestamp = 0,
			UpTime,
			Path,
			Value,
			ShortTime,
		};
		static const char* name(Enum e);
	};
public:
	using SnapShotFn = std::function<void (std::vector<ShvJournalEntry>&)>;

	FileShvJournal(std::string device_id, SnapShotFn snf);

	void setJournalDir(std::string s);
	const std::string& journalDir();
	void setFileSizeLimit(const std::string &n);
	void setFileSizeLimit(int64_t n) {m_fileSizeLimit = n;}
	int64_t fileSizeLimit() const { return m_fileSizeLimit;}
	void setJournalSizeLimit(const std::string &n);
	void setJournalSizeLimit(int64_t n) {m_journalSizeLimit = n;}
	int64_t journalSizeLimit() const { return m_journalSizeLimit;}
	void setDeviceId(std::string id) { m_deviceId = std::move(id); }
	void setDeviceType(std::string type) { m_deviceType = std::move(type); }
	void setTypeInfo(const shv::chainpack::RpcValue &i) { m_typeInfo = i; }

	void append(const ShvJournalEntry &entry, int64_t msec = 0);

	shv::chainpack::RpcValue getLog(const ShvJournalGetLogParams &params);
	//virtual std::string defaultJournaldir();
private:
	void checkJournalConsistecy();
	void rotateJournal();

	std::string fileNoToName(int n);
	void updateJournalStatus();
	void checkJournalDir();
	int64_t findLastEntryDateTime(const std::string &fn);

	void appendEntry(std::ofstream &out, int64_t msec, const ShvJournalEntry &e);

	std::string getLine(std::istream &in, char sep);
	//static long toLong(const std::string &s);
private:
	std::string m_deviceId;
	std::string m_deviceType;
	shv::chainpack::RpcValue m_typeInfo;
	struct //JournalDirStatus
	{
		bool journalDirExists = false;
		int minFileNo = -1;
		int maxFileNo = -1;
		int64_t journalSize = -1;
		int64_t recentTimeStamp = 0;

		bool isConsistent() const {return recentTimeStamp > 0 && maxFileNo >= 0 && journalDirExists && journalSize >= 0;}
	} m_journalStatus;
	SnapShotFn m_snapShotFn;
	std::string m_journalDir;
	int64_t m_fileSizeLimit = DEFAULT_FILE_SIZE_LIMIT;
	int64_t m_journalSizeLimit = DEFAULT_JOURNAL_SIZE_LIMIT;
	int m_getLogRecordCountLimit = DEFAULT_GET_LOG_RECORD_COUNT_LIMIT;
};

class SHVCORE_DECL_EXPORT FileShvJournal2
{
public:
	static constexpr long DEFAULT_FILE_SIZE_LIMIT = 100 * 1024;
	static constexpr long DEFAULT_JOURNAL_SIZE_LIMIT = 100 * 100 * 1024;
	static constexpr int DEFAULT_GET_LOG_RECORD_COUNT_LIMIT = 100 * 1000;
	static constexpr char FIELD_SEPARATOR = '\t';
	static constexpr char RECORD_SEPARATOR = '\n';

	static const char* KEY_NAME;
	static const char *KEY_RECORD_COUNT;
	static const char *KEY_PATHS_DICT;

	struct SHVCORE_DECL_EXPORT Column
	{
		enum Enum {
			Timestamp = 0,
			UpTime,
			Path,
			Value,
			ShortTime,
			Domain,
		};
		static const char* name(Enum e);
	};

	SHV_FIELD_IMPL(std::string, f, F, ileExtension)
public:
	using SnapShotFn = std::function<void (std::vector<ShvJournalEntry>&)>;

	FileShvJournal2(std::string device_id, SnapShotFn snf);

	void setJournalDir(std::string s);
	const std::string& journalDir();
	void setFileSizeLimit(const std::string &n);
	void setFileSizeLimit(int64_t n) {m_fileSizeLimit = n;}
	int64_t fileSizeLimit() const { return m_fileSizeLimit;}
	void setJournalSizeLimit(const std::string &n);
	void setJournalSizeLimit(int64_t n) {m_journalSizeLimit = n;}
	int64_t journalSizeLimit() const { return m_journalSizeLimit;}
	void setDeviceId(std::string id) { m_deviceId = std::move(id); }
	void setDeviceType(std::string type) { m_deviceType = std::move(type); }
	void setTypeInfo(const shv::chainpack::RpcValue &i) { m_typeInfo = i; }

	void append(const ShvJournalEntry &entry, int64_t msec = 0);

	shv::chainpack::RpcValue getLog(const ShvJournalGetLogParams &params);

	void convertLog1JournalDir();
private:
	void checkJournalConsistecy(bool force = false);
	void rotateJournal();
	void updateJournalStatus();
	void updateJournalFiles();
	void updateRecentTimeStamp();
	void ensureJournalDir();
	bool journalDirExists();
	int64_t findLastEntryDateTime(const std::string &fn);

	shv::chainpack::RpcValue getLogThrow(const ShvJournalGetLogParams &params);

	void appendThrow(const ShvJournalEntry &entry, int64_t msec);
	void appendEntry(std::ofstream &out, int64_t msec, const ShvJournalEntry &e);

	int64_t fileNameToFileMsec(const std::string &fn) const;
	std::string fileMsecToFileName(int64_t msec) const;
	std::string fileMsecToFilePath(int64_t file_msec) const;
	std::string getLine(std::istream &in, char sep);
private:
	std::string m_deviceId;
	std::string m_deviceType;
	shv::chainpack::RpcValue m_typeInfo;
	struct
	{
		bool journalDirExists = false;
		std::vector<int64_t> files;
		int64_t journalSize = -1;
		//bool isConsistent = false;
		int64_t lastFileSize = -1;
		int64_t recentTimeStamp = 0;

		bool isConsistent() const {return journalDirExists && journalSize >= 0;}
		//void setNotConsistent() {journalSize = -1;}
	} m_journalStatus;
	SnapShotFn m_snapShotFn;
	std::string m_journalDir;
	int64_t m_fileSizeLimit = DEFAULT_FILE_SIZE_LIMIT;
	int64_t m_journalSizeLimit = DEFAULT_JOURNAL_SIZE_LIMIT;
	int m_getLogRecordCountLimit = DEFAULT_GET_LOG_RECORD_COUNT_LIMIT;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif
