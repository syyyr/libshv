#ifndef SHV_IOTQT_UTILS_FILESHVJOURNAL_H
#define SHV_IOTQT_UTILS_FILESHVJOURNAL_H

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <functional>
#include <vector>

//namespace std { class istream; }

namespace shv {
namespace iotqt {
namespace utils {

struct SHVIOTQT_DECL_EXPORT ShvJournalEntry
{
	std::string path;
	shv::chainpack::RpcValue value;

	bool isValid() const {return !path.empty() && value.isValid();}
};

struct SHVIOTQT_DECL_EXPORT ShvJournalGetLogParams
{
	shv::chainpack::RpcValue::DateTime since;
	shv::chainpack::RpcValue::DateTime until;
	/// '*' and '**' wild-cards are supported
	/// '*' stands for single path segment, shv/pol/*/discon match shv/pol/ols/discon but not shv/pol/ols/depot/discon
	/// '**' stands for zero or more path segments, shv/pol/**/discon matches shv/pol/discon, shv/pol/ols/discon, shv/pol/ols/depot/discon
	std::string pathPattern;
	//enum class PatternType {None = 0, WildCard, RegExp};
	//PatternType patternType = PatternType::WildCard;
	enum class HeaderOptions : unsigned {
		BasicInfo = 1 << 0,
		FileldInfo = 1 << 1,
		TypeInfo = 1 << 2,
	};
	unsigned headerOptions = static_cast<unsigned>(HeaderOptions::BasicInfo);
	int maxRecordCount = 1000;
	bool withSnapshot = true;

	ShvJournalGetLogParams() {}
	ShvJournalGetLogParams(const shv::chainpack::RpcValue &opts);

	shv::chainpack::RpcValue toRpcValue() const;
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
public:
	using SnapShotFn = std::function<void (std::vector<ShvJournalEntry>&)>;

	FileShvJournal(SnapShotFn snf);

	void setJournalDir(std::string s) {m_journalDir = std::move(s);}
	const std::string& journalDir() const {return m_journalDir;}
	void setFileSizeLimit(long n) {m_fileSizeLimit = n;}
	long fileSizeLimit() const { return m_fileSizeLimit;}
	void setDirSizeLimit(long n) {m_dirSizeLimit = n;}
	long dirSizeLimit() const { return m_dirSizeLimit;}
	void setDeviceId(std::string id) { m_deviceId = std::move(id); }
	void setTypeInfo(const shv::chainpack::RpcValue &i) { m_typeInfo = i; }

	void append(const ShvJournalEntry &entry);

	shv::chainpack::RpcValue getLog(const ShvJournalGetLogParams &params);

	static bool pathMatch(const std::string &pattern, const std::string &path);
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
		long journalSize = -1;
	} m_journalDirStatus;
	SnapShotFn m_snapShotFn;
	std::string m_journalDir;
	long m_fileSizeLimit = DEFAULT_FILE_SIZE_LIMIT;
	long m_dirSizeLimit = DEFAULT_JOURNAL_SIZE_LIMIT;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif
