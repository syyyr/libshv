#include "shvlogfilereader.h"
#include "shvjournalentry.h"
#include "shvfilejournal.h"

#include "../exception.h"
#include "../string.h"
#include "../log.h"
#include "../stringview.h"

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

ShvLogFileReader::ShvLogFileReader(const std::string &file_name, const ShvLogHeader *header)
	: m_chainpackReader(m_ifstream)
{
	m_ifstream.open(file_name, std::ios::binary);
	if(!m_ifstream)
		SHV_EXCEPTION("Cannot open file " + file_name + " for reading.");

	cp::RpcValue::MetaData md;
	m_chainpackReader.read(md);
	if(header)
		m_logHeader = *header;
	else
		m_logHeader = ShvLogHeader::fromMetaData(md);

	m_pathsTypeDescr = m_logHeader.pathsTypeDescr();
	chainpack::ChainPackReader::ItemType t = m_chainpackReader.unpackNext();
	if(t != chainpack::ChainPackReader::ItemType::CCPCP_ITEM_LIST)
		SHV_EXCEPTION("Log is corrupted!");
}

ShvLogFileReader::~ShvLogFileReader()
{
}

bool ShvLogFileReader::next()
{
	using Column = ShvLogHeader::Column;
	while(true) {
		m_currentEntry = ShvJournalEntry();
		if(!m_ifstream)
			return false;
		if(m_chainpackReader.peekNext() == chainpack::ChainPackReader::ItemType::CCPCP_ITEM_CONTAINER_END)
			return false;

		cp::RpcValue val;
		m_chainpackReader.read(val);
		const chainpack::RpcValue::List &row = val.toList();
		int64_t time = row.value(Column::Timestamp).toDateTime().msecsSinceEpoch();
		cp::RpcValue p = row.value(Column::Path);
		if(p.isInt())
			p = m_logHeader.pathDictCRef().value(p.toInt());
		const std::string &path = p.toString();
		if(path.empty()) {
			logWShvJournal() << "Path dictionary corrupted, row:" << val.toCpon();
			continue;
		}
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

const ShvJournalEntry &ShvLogFileReader::entry()
{
	return m_currentEntry;
}

ShvLogTypeDescr::SampleType ShvLogFileReader::pathsSampleType(const std::string &path) const
{
	auto it = m_pathsTypeDescr.find(path);
	return it == m_pathsTypeDescr.end()? ShvJournalEntry::SampleType::Continuous: it->second.sampleType;
}


} // namespace utils
} // namespace core
} // namespace shv
