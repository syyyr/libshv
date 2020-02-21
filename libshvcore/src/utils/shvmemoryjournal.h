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

	void setSince(int64_t since) { m_logHeader.setSince(since); }
	void setUntil(int64_t until) { m_logHeader.setUntil(until); }
	void setTypeInfo(const ShvLogTypeInfo &ti) {setTypeInfo(ShvLogTypeInfo(ti));}
	void setTypeInfo(ShvLogTypeInfo &&ti) {m_logHeader.setTypeInfo(std::move(ti));}
	void setTypeInfo(const std::string &path_prefix, ShvLogTypeInfo &&ti) {m_logHeader.setTypeInfo(path_prefix, std::move(ti));}
	void setTypeInfo(const std::string &path_prefix, const ShvLogTypeInfo &ti) {setTypeInfo(path_prefix, ShvLogTypeInfo(ti));}
	void setDeviceId(std::string id) { m_logHeader.setDeviceId(std::move(id)); }
	void setDeviceType(std::string type) { m_logHeader.setDeviceType(std::move(type)); }

	void append(const ShvJournalEntry &entry) override;
	int inputFilterRecordCountLimit() const { return  m_inputFilterRecordCountLimit; }

	void loadLog(const shv::chainpack::RpcValue &log, bool append_records = false);
	shv::chainpack::RpcValue getLog(const ShvGetLogParams &params) override;

	//const ShvLogHeader &logHeader() const { return m_logHeader; }
	const std::vector<ShvJournalEntry>& entries() const {return  m_entries;}
	size_t size() const { return  m_entries.size(); }
	void clear() { m_entries.clear(); }
private:
	void checkSampleType(ShvJournalEntry &entry) const;
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
	std::map<std::string, ShvLogTypeDescr> m_pathsTypeDescr;

	std::vector<Entry> m_entries;
};

} // namespace utils
} // namespace core
} // namespace shv

