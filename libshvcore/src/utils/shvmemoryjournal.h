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

	void setSince(const shv::chainpack::RpcValue &since) { m_logHeader.setSince(since); }
	void setUntil(const shv::chainpack::RpcValue &until) { m_logHeader.setUntil(until); }
	void setTypeInfo(const ShvLogTypeInfo &ti, const std::string &path_prefix = ShvLogHeader::EMPTY_PREFIX_KEY) {setTypeInfo(ShvLogTypeInfo(ti), path_prefix);}
	void setTypeInfo(ShvLogTypeInfo &&ti, const std::string &path_prefix = ShvLogHeader::EMPTY_PREFIX_KEY) {m_logHeader.setTypeInfo(std::move(ti), path_prefix);}
	void setDeviceId(std::string id) { m_logHeader.setDeviceId(std::move(id)); }
	void setDeviceType(std::string type) { m_logHeader.setDeviceType(std::move(type)); }

	bool isShortTimeCorrection() const { return m_isShortTimeCorrection; }
	void setShortTimeCorrection(bool b) { m_isShortTimeCorrection = b; }

	void append(const ShvJournalEntry &entry) override;
	int inputFilterRecordCountLimit() const { return  m_inputFilterRecordCountLimit; }

	void loadLog(const shv::chainpack::RpcValue &log, bool append_records = false);
	shv::chainpack::RpcValue getLog(const ShvGetLogParams &params) override;

	// we do not expose whole header, since append() does not update field until
	//const ShvLogHeader &logHeader() const { return m_logHeader; }
	bool hasSnapshot() const { return m_logHeader.withSnapShot(); }

	const std::vector<ShvJournalEntry>& entries() const {return  m_entries;}
	size_t size() const { return  m_entries.size(); }
	const ShvJournalEntry& at(size_t ix) const { return  m_entries.at(ix); }
	void clear() { m_entries.clear(); }
	//size_t timeToUpperBoundIndex(int64_t time) const;
private:
	using Entry = ShvJournalEntry;

	ShvGetLogParams m_inputFilter;
	PatternMatcher m_patternMatcher;
	int64_t m_inputFilterSinceMsec = 0;
	int64_t m_inputFilterUntilMsec = 0;
	int m_inputFilterRecordCountLimit = DEFAULT_GET_LOG_RECORD_COUNT_LIMIT;

	std::map<std::string, Entry> m_inputSnapshot;

	std::map<std::string, int> m_pathDictionary;
	int m_pathDictionaryIndex = 0;

	ShvLogHeader m_logHeader;
	std::map<std::string, ShvLogTypeDescr> m_pathsTypeDescr;

	std::vector<Entry> m_entries;


	struct ShortTime {
		int64_t epochTime = 0;
		uint16_t recentShortTime = 0;

		uint16_t shortTimeDiff(uint16_t msec) const
		{
			return static_cast<uint16_t>(msec - recentShortTime);
		}
		int64_t toEpochTime(uint16_t msec) const
		{
			return epochTime + shortTimeDiff(msec);
		}
		int64_t addShortTime(uint16_t msec)
		{
			epochTime = toEpochTime(msec);
			recentShortTime = msec;
			return epochTime;
		}
	};

	bool m_isShortTimeCorrection = false;
	std::map<std::string, ShortTime> m_recentShortTimes;};

} // namespace utils
} // namespace core
} // namespace shv

