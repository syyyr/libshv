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

struct SHVIOTQT_DECL_EXPORT ShvJournalGetOptions
{
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

	ShvJournalGetOptions() {}
	ShvJournalGetOptions(const shv::chainpack::RpcValue &opts);
};

class SHVIOTQT_DECL_EXPORT FileShvJournal
{
public:
	using SnapShotFn = std::function<void (std::vector<ShvJournalEntry>&)>;

	FileShvJournal(SnapShotFn snf);

	void setJournalDir(std::string s) {m_journalDir = std::move(s);}
	void setFileSizeLimit(long n) {m_fileSizeLimit = n;}
	void setDeviceId(std::string id) { m_deviceId = std::move(id); }
	void setTypeInfo(const shv::chainpack::RpcValue &i) { m_typeInfo = i; }

	void append(const ShvJournalEntry &entry);

	shv::chainpack::RpcValue getLog(const chainpack::RpcValue::DateTime &since, const chainpack::RpcValue::DateTime &until, const ShvJournalGetOptions &opts = ShvJournalGetOptions());

	static bool pathMatch(const std::string &pattern, const std::string &path);
private:
	std::string fileNoToName(int n);
	long fileSize(const std::string &fn);
	int lastFileNo();
	void setLastFileNo(int n) {m_lastFileNo = n;}
	int64_t findLastEntryDateTime(const std::string &fn);
	void appendEntry(std::ofstream &out, int64_t msec, int uptime_sec, const ShvJournalEntry &e);

	//int findFileWithSnapshotFor(const chainpack::RpcValue::DateTime &from);

	std::string getLine(std::istream &in, char sep);
	static long toLong(const std::string &s);
private:
	static constexpr long DEFAULT_FILE_SIZE_LIMIT = 100 * 1024;
	static constexpr int FILE_DIGITS = 4;
	static const char* FILE_EXT;
	static constexpr char FIELD_SEPARATOR = '\t';
	static constexpr char RECORD_SEPARATOR = '\n';
	std::string m_deviceId;
	shv::chainpack::RpcValue m_typeInfo;
	int m_lastFileNo = -1;
	SnapShotFn m_snapShotFn;
	std::string m_journalDir;
	long m_fileSizeLimit = DEFAULT_FILE_SIZE_LIMIT;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif
