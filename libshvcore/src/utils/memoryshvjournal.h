#ifndef SHV_CORE_UTILS_MEMORYSHVJOURNAL_H
#define SHV_CORE_UTILS_MEMORYSHVJOURNAL_H

#include "../shvcoreglobal.h"

#include "abstractshvjournal.h"
#include "shvjournalentry.h"
#include "shvjournalgetlogparams.h"

#include <regex>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT MemoryShvJournal : public AbstractShvJournal
{
public:
	MemoryShvJournal();
	MemoryShvJournal(const ShvJournalGetLogParams &input_filter);

	void setTypeInfo(const shv::chainpack::RpcValue &i);
	void setTypeInfo(const shv::chainpack::RpcValue &i, const std::string &path_prefix);
	void setDeviceId(std::string id) { m_deviceId = std::move(id); }
	void setDeviceType(std::string type) { m_deviceType = std::move(type); }

	void loadLog(const shv::chainpack::RpcValue &log);
	void append(const ShvJournalEntry &entry) override;
	shv::chainpack::RpcValue getLog(const ShvJournalGetLogParams &params) override;

	const std::vector<ShvJournalEntry>& entries() const {return  m_entries;}
private:
	using Entry = ShvJournalEntry;
	void append(Entry &&entry);

	PatternMatcher m_patternMatcher;
	ShvJournalGetLogParams m_inputFilter;

	std::map<std::string, Entry> m_snapshot;

	int64_t m_sinceMsec = 0;
	int64_t m_untilMsec = 0;
	int m_maxRecordCount = DEFAULT_GET_LOG_RECORD_COUNT_LIMIT;

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
