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

class SHVCORE_DECL_EXPORT FileShvJournal2 : public AbstractShvJournal
{
public:
	static constexpr long DEFAULT_JOURNAL_SIZE_LIMIT = 100 * 100 * 1024;
	static constexpr char FIELD_SEPARATOR = '\t';
	static constexpr char RECORD_SEPARATOR = '\n';
	static const std::string FILE_EXT;
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
	void setTypeInfo(const ShvLogTypeInfo &i) { m_journalContext.typeInfo = i; }
	void setDeviceId(std::string id) { m_journalContext.deviceId = std::move(id); }
	void setDeviceType(std::string type) { m_journalContext.deviceType = std::move(type); }

	void append(const ShvJournalEntry &entry) override;

	shv::chainpack::RpcValue getLog(const ShvGetLogParams &params) override;

	void convertLog1JournalDir();
public:
	struct JournalContext
	{
		bool journalDirExists = false;
		std::vector<int64_t> files;
		int64_t journalSize = -1;
		//bool isConsistent = false;
		int64_t lastFileSize = -1;
		int64_t recentTimeStamp = 0;
		std::string journalDir;

		std::string deviceId;
		std::string deviceType;
		ShvLogTypeInfo typeInfo;

		bool isConsistent() const {return journalDirExists && journalSize >= 0;}
		//void setNotConsistent() {journalSize = -1;}
		int64_t fileNameToFileMsec(const std::string &fn) const;
		std::string fileMsecToFileName(int64_t msec) const;
		std::string fileMsecToFilePath(int64_t file_msec) const;
	};
	const JournalContext& checkJournalContext();
	static shv::chainpack::RpcValue getLog(const JournalContext &journal_context, const ShvGetLogParams &params);
private:

	void checkJournalContext_helper(bool force = false);

	void rotateJournal();
	void updateJournalStatus();
	void updateJournalFiles();
	void updateRecentTimeStamp();
	void ensureJournalDir();
	bool journalDirExists();
	int64_t findLastEntryDateTime(const std::string &fn);

	void appendThrow(const ShvJournalEntry &entry);
	void wrirteEntry(std::ofstream &out, int64_t msec, const ShvJournalEntry &e);

	static std::string getLine(std::istream &in, char sep);
private:
	JournalContext m_journalContext;

	SnapShotFn m_snapShotFn;
	int64_t m_fileSizeLimit = DEFAULT_FILE_SIZE_LIMIT;
	int64_t m_journalSizeLimit = DEFAULT_JOURNAL_SIZE_LIMIT;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_FILESHVJOURNAL2_H
