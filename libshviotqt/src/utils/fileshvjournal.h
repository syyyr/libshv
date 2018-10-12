#ifndef SHV_IOTQT_UTILS_FILESHVJOURNAL_H
#define SHV_IOTQT_UTILS_FILESHVJOURNAL_H

#include "../shviotqtglobal.h"

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

	bool isValid() const {return !path.empty() && value.isValid();}
};

class SHVIOTQT_DECL_EXPORT FileShvJournal
{
public:
	using SnapShotFn = std::function<void (std::vector<ShvJournalEntry>&)>;

	FileShvJournal(SnapShotFn snf);

	void setJournalDir(std::string s) {m_journalDir = std::move(s);}
	void setFileSizeLimit(long n) {m_fileSizeLimit = n;}

	void append(const ShvJournalEntry &entry);
private:
	std::string fileNoToName(int n);
	long fileSize(const std::string &fn);
	int lastFileNo();
	void setLastFileNo(int n) {m_lastFileNo = n;}
	int64_t findLastEntryDateTime(const std::string &fn);
	void appendEntry(std::ofstream &out, int64_t msec, int uptime_sec, const ShvJournalEntry &e);
private:
	static constexpr int FILE_DIGITS = 4;
	static const char* FILE_EXT;
	static constexpr char FIELD_SEPARATOR = '\t';
	static constexpr char RECORD_SEPARATOR = '\n';
	int m_lastFileNo = -1;
	SnapShotFn m_snapShotFn;
	std::string m_journalDir;
	long m_fileSizeLimit = 100 * 1024;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif
