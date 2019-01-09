#ifndef SHV_IOTQT_UTILS_FILESHVJOURNAL_H
#define SHV_IOTQT_UTILS_FILESHVJOURNAL_H

#include "../shviotqtglobal.h"
#include "shvjournalgetlogparams.h"

#include <shv/chainpack/rpcvalue.h>

#include <functional>
#include <vector>

namespace shv {
namespace iotqt {
namespace utils {

struct SHVIOTQT_DECL_EXPORT ShvJournalEntry
{
	std::string path;
	shv::chainpack::RpcValue value;

	ShvJournalEntry() {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value)
		: path(std::move(path))
		, value{value}
	{}
	bool isValid() const {return !path.empty() && value.isValid();}
};

class SHVIOTQT_DECL_EXPORT FileShvJournal
{
public:
	static constexpr long DEFAULT_FILE_SIZE_LIMIT = 100 * 1024;
	static constexpr long DEFAULT_JOURNAL_SIZE_LIMIT = 100 * 100 * 1024;
	static constexpr int FILE_DIGITS = 6;
	static const char* FILE_EXT;
	static constexpr char FIELD_SEPARATOR = '\t';
	static constexpr char RECORD_SEPARATOR = '\n';

	struct Column
	{
		enum Enum {
			Timestamp = 0,
			Uptime,
			Path,
			Value,
		};
	};
public:
	using SnapShotFn = std::function<void (std::vector<ShvJournalEntry>&)>;

	FileShvJournal(SnapShotFn snf);

	void setJournalDir(std::string s) {m_journalDir = std::move(s);}
	const std::string& journalDir() const {return m_journalDir;}
	void setFileSizeLimit(const std::string &n);
	void setFileSizeLimit(int64_t n) {m_fileSizeLimit = n;}
	int64_t fileSizeLimit() const { return m_fileSizeLimit;}
	void setJournalSizeLimit(const std::string &n);
	void setJournalSizeLimit(int64_t n) {m_journalSizeLimit = n;}
	int64_t journalSizeLimit() const { return m_journalSizeLimit;}
	void setDeviceId(std::string id) { m_deviceId = std::move(id); }
	void setTypeInfo(const shv::chainpack::RpcValue &i) { m_typeInfo = i; }

	void append(const ShvJournalEntry &entry, int64_t msec = 0);

	shv::chainpack::RpcValue getLog(const ShvJournalGetLogParams &params);
private:
	std::string fileNoToName(int n);
	//long fileSize(const std::string &fn);
	int lastFileNo();
	void updateJournalDirStatus();
	void setLastFileNo(int n) {m_journalDirStatus.maxFileNo = n;}
	int64_t findLastEntryDateTime(const std::string &fn);
	void appendEntry(std::ofstream &out, int64_t msec, int uptime_sec, const ShvJournalEntry &e);

	void checkJournalDir();
	void rotateJournal();

	std::string getLine(std::istream &in, char sep);
	static long toLong(const std::string &s);
private:
	std::string m_deviceId;
	shv::chainpack::RpcValue m_typeInfo;
	struct //JournalDirStatus
	{
		int minFileNo = -1;
		int maxFileNo = -1;
		int64_t journalSize = -1;
	} m_journalDirStatus;
	SnapShotFn m_snapShotFn;
	std::string m_journalDir;
	int64_t m_fileSizeLimit = DEFAULT_FILE_SIZE_LIMIT;
	int64_t m_journalSizeLimit = DEFAULT_JOURNAL_SIZE_LIMIT;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif
