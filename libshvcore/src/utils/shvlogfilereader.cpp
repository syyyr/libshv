#include "shvlogfilereader.h"
#include "shvjournalentry.h"

#include "../exception.h"
#include "../log.h"

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

ShvLogFileReader::ShvLogFileReader(chainpack::ChainPackReader *reader)
	: m_reader(reader)
{
	init();
}

ShvLogFileReader::ShvLogFileReader(const std::string &file_name)
{
	m_readerCreated = true;
	m_ifstream = new std::ifstream;
	m_ifstream->open(file_name, std::ios::binary);
	if(!m_ifstream)
		SHV_EXCEPTION("Cannot open file " + file_name + " for reading.");
	m_reader = new shv::chainpack::ChainPackReader(*m_ifstream);
	init();
}

void ShvLogFileReader::init()
{
	cp::RpcValue::MetaData md;
	m_reader->read(md);
	m_logHeader = ShvLogHeader::fromMetaData(md);

	chainpack::ChainPackReader::ItemType t = m_reader->unpackNext();
	if(t != chainpack::ChainPackReader::ItemType::CCPCP_ITEM_LIST)
		SHV_EXCEPTION("Log is corrupted!");
}

ShvLogFileReader::~ShvLogFileReader()
{
	if(m_readerCreated) {
		if(m_reader)
			delete m_reader;
		if(m_ifstream)
			delete m_ifstream;
	}
}

bool ShvLogFileReader::next()
{
	using Column = ShvLogHeader::Column;
	while(true) {
		m_currentEntry = ShvJournalEntry();
		if(!m_ifstream)
			return false;
		chainpack::ChainPackReader::ItemType tt = m_reader->peekNext();
		//logDShvJournal() << "peek next type:" << chainpack::ChainPackReader::itemTypeToString(tt);
		if(tt == chainpack::ChainPackReader::ItemType::CCPCP_ITEM_CONTAINER_END)
			return false;

		cp::RpcValue val;
		m_reader->read(val);
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
		const std::string &path = p.asString();
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
		m_currentEntry.domain = row.value(Column::Domain).asString();
		if (m_currentEntry.domain.empty()) {
			m_currentEntry.domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
		}
		m_currentEntry.valueFlags = row.value(Column::ValueFlags).toUInt();
		m_currentEntry.userId = row.value(Column::UserId).asString();
		return true;
	}
}

const ShvJournalEntry &ShvLogFileReader::entry()
{
	return m_currentEntry;
}

} // namespace utils
} // namespace core
} // namespace shv
