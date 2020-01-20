#ifndef SHV_CORE_UTILS_MEMORYSHVJOURNAL_H
#define SHV_CORE_UTILS_MEMORYSHVJOURNAL_H

#include "../shvcoreglobal.h"

#include "shvjournalcommon.h"
#include "shvjournalentry.h"
#include "shvjournalgetlogparams.h"

#include <regex>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT MemoryShvJournal : public ShvJournalCommon
{
public:
	MemoryShvJournal();
	MemoryShvJournal(const ShvJournalGetLogParams &input_filter);

	void setDeviceId(std::string id) { m_deviceId = std::move(id); }
	void setDeviceType(std::string type) { m_deviceType = std::move(type); }
	void setTypeInfo(const shv::chainpack::RpcValue &i);
	void setTypeInfo(const shv::chainpack::RpcValue &i, const std::string &path_prefix);

	void loadLog(const shv::chainpack::RpcValue &log);
	void append(const ShvJournalEntry &entry, int64_t epoch_msec = 0);
	shv::chainpack::RpcValue getLog(const ShvJournalGetLogParams &params);
private:
	class Entry : public ShvJournalEntry
	{
	public:
		int64_t timeMsec = 0;

		Entry() {}
		Entry(const ShvJournalEntry &e) : ShvJournalEntry(e) {}
	};
	void append(Entry &&entry);

	std::regex m_pathPatternRegEx;
	std::string m_pathPatternWildCard;
	bool m_usePathPatternregEx = false;

	std::regex m_domainPatternRegEx;
	bool m_useDomainPatternregEx = false;

	int64_t m_sinceMsec = 0;
	int64_t m_untilMsec = 0;
	int m_maxRecordCount = std::numeric_limits<int>::max();

	std::map<std::string, int> m_pathDictionary;
	int m_pathDictionaryIndex = 0;

	std::string m_deviceId;
	std::string m_deviceType;
	shv::chainpack::RpcValue::Map m_typeInfos;

	std::vector<Entry> m_entries;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_MEMORYSHVJOURNAL_H
