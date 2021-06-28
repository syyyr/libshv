#ifndef SHV_CORE_UTILS_FILESHVJOURNAL2_H
#define SHV_CORE_UTILS_FILESHVJOURNAL2_H

#include "../shvcoreglobal.h"
#include "../utils.h"
#include "abstractshvjournal.h"
#include "shvjournalentry.h"
#include "shvgetlogparams.h"

#include <functional>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvFileJournal : public AbstractShvJournal
{
public:
	static constexpr long DEFAULT_FILE_SIZE_LIMIT = 1024 * 1024;
	static constexpr long DEFAULT_JOURNAL_SIZE_LIMIT = 100 * DEFAULT_FILE_SIZE_LIMIT;
	static constexpr char FIELD_SEPARATOR = '\t';
	static constexpr char RECORD_SEPARATOR = '\n';
	static const std::string FILE_EXT;
public:
	using SnapShot = std::vector<ShvJournalEntry>;
	using SnapShotFn = std::function<void (SnapShot &snapshot)>;
	using TSNowFn = std::function<int64_t ()>;

	ShvFileJournal(std::string device_id, SnapShotFn snf);

	void setJournalDir(std::string s);
	const std::string& journalDir();
	void setFileSizeLimit(const std::string &n);
	void setFileSizeLimit(int64_t n) {m_fileSizeLimit = n;}
	int64_t fileSizeLimit() const { return m_fileSizeLimit;}
	void setJournalSizeLimit(const std::string &n);
	void setJournalSizeLimit(int64_t n) {m_journalSizeLimit = n;}
	int64_t journalSizeLimit() const { return m_journalSizeLimit;}
	void setTypeInfo(const ShvLogTypeInfo &i) { m_journalContext.typeInfo = i; }
	const ShvLogTypeInfo& typeInfo() const { return m_journalContext.typeInfo; }
	std::string deviceId() const { return m_journalContext.deviceId; }
	void setDeviceId(std::string id) { m_journalContext.deviceId = std::move(id); }
	std::string deviceType() const { return m_journalContext.deviceType; }
	void setDeviceType(std::string type) { m_journalContext.deviceType = std::move(type); }

	static int64_t findLastEntryDateTime(const std::string &fn, ssize_t *p_date_time_fpos = nullptr);
	void append(const ShvJournalEntry &entry) override;

	// testing purposes
	//void setAppendLogTSNowFn(TSNowFn fn) { m_appendLogTSNowFn = fn; }
	//void setDefaultAppendLogTSNowFn();

	shv::chainpack::RpcValue getLog(const ShvGetLogParams &params) override;
	shv::chainpack::RpcValue getSnapShotMap() override;

	void convertLog1JournalDir();
public:
	struct TxtColumn
	{
		enum Enum {
			Timestamp = 0,
			UpTime,
			Path,
			Value,
			ShortTime,
			Domain,
			SampleType,
			UserId,
		};
		static const char* name(Enum e);
	};
	struct JournalContext
	{
		bool journalDirExists = false;
		std::vector<int64_t> files;
		int64_t journalSize = -1;
		int64_t lastFileSize = -1;
		int64_t recentTimeStamp = 0;
		std::string journalDir;

		std::string deviceId;
		std::string deviceType;
		ShvLogTypeInfo typeInfo;

		bool isConsistent() const {return journalDirExists && journalSize >= 0;}
		//void setNotConsistent() {journalSize = -1;}
		static int64_t fileNameToFileMsec(const std::string &fn);
		static std::string msecToBaseFileName(int64_t msec);
		static std::string fileMsecToFileName(int64_t msec);
		std::string fileMsecToFilePath(int64_t file_msec) const;
	};
	const JournalContext& checkJournalContext();
	void createNewLogFile(int64_t journal_file_start_msec = 0);
	static shv::chainpack::RpcValue getLog(const JournalContext &journal_context, const ShvGetLogParams &params);
private:

	void checkJournalContext_helper(bool force = false);

	void rotateJournal();
	void updateJournalStatus();
	void checkRecentTimeStamp();
	void ensureJournalDir();
	bool journalDirExists();

	void appendThrow(const ShvJournalEntry &entry);
private:
	JournalContext m_journalContext;

	SnapShotFn m_snapShotFn;
	int64_t m_fileSizeLimit = DEFAULT_FILE_SIZE_LIMIT;
	int64_t m_journalSizeLimit = DEFAULT_JOURNAL_SIZE_LIMIT;

	// we need custom DateTime::now() fn for testing purposes
	//TSNowFn m_appendLogTSNowFn;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_FILESHVJOURNAL2_H
