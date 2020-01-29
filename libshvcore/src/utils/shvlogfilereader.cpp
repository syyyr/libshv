#include "shvlogfilereader.h"
#include "shvjournalentry.h"
#include "abstractshvjournal.h"

#include "../exception.h"
#include "../string.h"
#include "../log.h"
#include "../stringview.h"

#include <shv/chainpack/chainpackreader.h>

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

ShvLogFileReader::ShvLogFileReader(const std::string &file_name)
	: ShvLogFileReader(file_name, ShvLogHeader())
{
}

ShvLogFileReader::ShvLogFileReader(const std::string &file_name, const ShvLogHeader &header)
	: m_logHeader(header)
{
	m_ifstream.open(file_name, std::ios::binary);
	if(!m_ifstream)
		SHV_EXCEPTION("Cannot open file " + file_name + " for reading.");
	m_isTextLog = String::endsWith(file_name, ".log2");
	if(m_isTextLog) {
		if(m_logHeader.typeInfos().empty()) {
			shvWarning() << "Type info not provided for text log, sample type cannot be discovered.";
		}
	}
	else {
		m_chainpackReader = new cp::ChainPackReader(m_ifstream);
		cp::RpcValue::MetaData md;
		m_chainpackReader->read(md);
		if(m_logHeader.typeInfos().empty()) {
			m_logHeader = ShvLogHeader::fromMetaData(md);
		}
		m_colTimestamp = AbstractShvJournal::Column::index(AbstractShvJournal::Column::Timestamp, m_logHeader.withUptime());
		m_colPath = AbstractShvJournal::Column::index(AbstractShvJournal::Column::Path, m_logHeader.withUptime());
		m_colValue = AbstractShvJournal::Column::index(AbstractShvJournal::Column::Value, m_logHeader.withUptime());
		m_colShortTime = AbstractShvJournal::Column::index(AbstractShvJournal::Column::ShortTime, m_logHeader.withUptime());
		m_colDomain = AbstractShvJournal::Column::index(AbstractShvJournal::Column::Domain, m_logHeader.withUptime());
		chainpack::ChainPackReader::ItemType t = m_chainpackReader->unpackNext();
		if(t != chainpack::ChainPackReader::ItemType::CCPCP_ITEM_LIST)
			SHV_EXCEPTION("Log is corrupted!");
	}
	m_pathsSampleTypes = m_logHeader.pathsSampleTypes();
}

ShvLogFileReader::~ShvLogFileReader()
{
	if(m_chainpackReader)
		delete m_chainpackReader;
}

static std::string getLine(std::istream &in, char sep)
{
	std::string line;
	while(in) {
		char buff[1024];
		in.getline(buff, sizeof (buff), sep);
		std::string s(buff);
		line += s;
		if(in.gcount() == (long)s.size()) {
			// separator not found
			continue;
		}
		break;
	}
	return line;
}

ShvJournalEntry ShvLogFileReader::next()
{
	ShvJournalEntry e;
	if(m_chainpackReader) {
		while(true) {
			cp::RpcValue val;
			m_chainpackReader->read(val);
			const chainpack::RpcValue::List &row = val.toList();
			int64_t time = row.value(m_colTimestamp).toDateTime().msecsSinceEpoch();
			cp::RpcValue p = row.value(m_colPath);
			if(p.isInt())
				p = m_logHeader.pathDictCRef().value(p.toInt());
			const std::string &path = p.toString();
			if(path.empty()) {
				logWShvJournal() << "Path dictionary corrupted, row:" << val.toCpon();
				continue;
			}
			e.epochMsec = time;
			e.path = path;
			e.value = row.value(m_colValue);
			cp::RpcValue st = row.value(m_colShortTime);
			e.shortTime = st.isInt() && st.toInt() >= 0? st.toInt(): ShvJournalEntry::NO_SHORT_TIME;
			e.domain = row.value(m_colDomain).toString();
			e.sampleType = pathsSampleType(e.path);
			break;
		}
	}
	else while(true) {
		if(!m_ifstream)
			return ShvJournalEntry();

		std::string line = getLine(m_ifstream, '\n');
		shv::core::StringView sv(line);
		shv::core::StringViewList line_record = sv.split('\t', shv::core::StringView::KeepEmptyParts);
		if(line_record.empty()) {
			logDShvJournal() << "skipping empty line";
			continue; // skip empty line
		}
		std::string dtstr = line_record[AbstractShvJournal::Column::Timestamp].toString();
		std::string path = line_record.value(AbstractShvJournal::Column::Path).toString();
		std::string domain = line_record.value(AbstractShvJournal::Column::Domain).toString();
		size_t len;
		cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromUtcString(dtstr, &len);
		if(len == 0) {
			logWShvJournal() << "invalid date time string:" << dtstr << "line will be ignored";
			continue;
		}
		StringView short_time_sv = line_record.value(AbstractShvJournal::Column::ShortTime);

		e.path = std::move(path);
		e.epochMsec = dt.msecsSinceEpoch();
		e.shortTime = short_time_sv.toInt();
		e.domain = std::move(domain);
		std::string err;
		e.value = cp::RpcValue::fromCpon(line_record.value(AbstractShvJournal::Column::Value).toString(), &err);
		if(!err.empty())
			logWShvJournal() << "Invalid CPON value:" << line_record.value(AbstractShvJournal::Column::Value).toString();
		e.sampleType = pathsSampleType(e.path);
		break;
	}
	return e;
}

ShvLogTypeDescription::SampleType ShvLogFileReader::pathsSampleType(const std::string &path) const
{
	auto it = m_pathsSampleTypes.find(path);
	return it == m_pathsSampleTypes.end()? ShvJournalEntry::SampleType::Continuous: it->second;
}


} // namespace utils
} // namespace core
} // namespace shv
