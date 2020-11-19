#include "shvlogrpcvaluereader.h"

#include "../exception.h"
#include "../log.h"

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

ShvLogRpcValueReader::ShvLogRpcValueReader(const shv::chainpack::RpcValue &log, bool throw_exceptions)
	: m_log(log)
	, m_isThrowExceptions(throw_exceptions)
{
	m_logHeader = ShvLogHeader::fromMetaData(m_log.metaData());

	if(!m_log.isList() && m_isThrowExceptions)
		SHV_EXCEPTION("Log is corrupted!");
}

bool ShvLogRpcValueReader::next()
{
	using Column = ShvLogHeader::Column;
	while(true) {
		m_currentEntry = ShvJournalEntry();
		const chainpack::RpcValue::List &list = m_log.toList();
		if(m_currentIndex >= list.size())
			return false;

		const cp::RpcValue &val = list[m_currentIndex++];
		const chainpack::RpcValue::List &row = val.toList();
		cp::RpcValue dt = row.value(Column::Timestamp);
		if(!dt.isDateTime()) {
			if(m_isThrowExceptions)
				throw shv::core::Exception("Invalid date time, row: " + val.toCpon());
			else
				logWShvJournal() << "Skipping invalid date time, row:" << val.toCpon();
			continue;
		}
		int64_t time = dt.toDateTime().msecsSinceEpoch();
		cp::RpcValue p = row.value(Column::Path);
		if(p.isInt())
			p = m_logHeader.pathDictCRef().value(p.toInt());
		const std::string &path = p.toString();
		if(path.empty()) {
			if(m_isThrowExceptions)
				throw shv::core::Exception("Path dictionary corrupted, row: " + val.toCpon());
			else
				logWShvJournal() << "Path dictionary corrupted, row:" << val.toCpon();
			continue;
		}
		//logDShvJournal() << "row:" << val.toCpon();
		m_currentEntry.epochMsec = time;
		m_currentEntry.path = path;
		m_currentEntry.value = row.value(Column::Value);
		cp::RpcValue st = row.value(Column::ShortTime);
		m_currentEntry.shortTime = st.isInt() && st.toInt() >= 0? st.toInt(): ShvJournalEntry::NO_SHORT_TIME;
		m_currentEntry.domain = row.value(Column::Domain).toString();
		m_currentEntry.sampleType = static_cast<ShvJournalEntry::SampleType>(row.value(Column::SampleType).toUInt());
		if (m_currentEntry.sampleType == ShvJournalEntry::SampleType::Invalid) {
			m_currentEntry.sampleType = ShvJournalEntry::SampleType::Continuous;
		}
		return true;
	}
}

} // namespace utils
} // namespace core
} // namespace shv
