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

ShvLogRpcValueReader::ShvLogRpcValueReader(const shv::chainpack::RpcValue &log, const ShvLogHeader *header)
	: m_log(log)
{
	if(header)
		m_logHeader = *header;
	else
		m_logHeader = ShvLogHeader::fromMetaData(m_log.metaData());

	if(!m_log.isList())
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
			logWShvJournal() << "Skipping invalid date time, row:" << val.toCpon();
			continue;
		}
		int64_t time = dt.toDateTime().msecsSinceEpoch();
		cp::RpcValue p = row.value(Column::Path);
		if(p.isInt())
			p = m_logHeader.pathDictCRef().value(p.toInt());
		const std::string &path = p.toString();
		if(path.empty()) {
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
		m_currentEntry.sampleType = pathsSampleType(m_currentEntry.path);
		return true;
	}
}

ShvLogTypeDescr::SampleType ShvLogRpcValueReader::pathsSampleType(const std::string &path) const
{
	ShvLogTypeDescr::SampleType st = m_logHeader.pathsSampleType(path);
	if(st == ShvLogTypeDescr::SampleType::Invalid)
		return ShvLogTypeDescr::SampleType::Continuous;
	return st;
}

} // namespace utils
} // namespace core
} // namespace shv
