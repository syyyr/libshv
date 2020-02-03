#pragma once

#include "../shvcoreglobal.h"

#include "abstractshvjournal.h"
#include "shvjournalentry.h"
#include "shvgetlogparams.h"
#include "shvlogheader.h"

#include <regex>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvMemoryJournal : public AbstractShvJournal
{
public:
	ShvMemoryJournal();
	ShvMemoryJournal(const ShvGetLogParams &input_filter);

	void setTypeInfo(ShvLogTypeInfo &&ti) {m_logHeader.setTypeInfo(std::move(ti));}
	void setTypeInfo(const std::string &path_prefix, ShvLogTypeInfo &&ti) {m_logHeader.setTypeInfo(path_prefix, std::move(ti));}
	void setDeviceId(std::string id) { m_logHeader.setDeviceId(std::move(id)); }
	void setDeviceType(std::string type) { m_logHeader.setDeviceType(std::move(type)); }

	void loadLog(const shv::chainpack::RpcValue &log);
	void append(const ShvJournalEntry &entry) override;
	shv::chainpack::RpcValue getLog(const ShvGetLogParams &params) override;

	const std::vector<ShvJournalEntry>& entries() const {return  m_entries;}
private:
	using Entry = ShvJournalEntry;

	ShvGetLogParams m_inputFilter;
	PatternMatcher m_patternMatcher;
	int64_t m_inputFilterSinceMsec = 0;
	int64_t m_inputFilterUntilMsec = 0;
	int m_inputFilterRecordCountLimit = DEFAULT_GET_LOG_RECORD_COUNT_LIMIT;

	std::map<std::string, Entry> m_snapshot;

	std::map<std::string, int> m_pathDictionary;
	int m_pathDictionaryIndex = 0;

	ShvLogHeader m_logHeader;

	std::vector<Entry> m_entries;
};

} // namespace utils
} // namespace core
} // namespace shv

