#include "shvlogfilereader.h"
#include "shvjournalentry.h"
#include "fileshvjournal2.h"

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
	m_pathsTypeDescr = m_logHeader.pathsTypeDescr();
	m_isTextLog = String::endsWith(file_name, ".log2");
	if(m_isTextLog) {
		if(m_pathsTypeDescr.empty()) {
			shvWarning() << "Type info not provided for text log, sample type cannot be discovered.";
		}
	}
	else {
		m_chainpackReader = new cp::ChainPackReader(m_ifstream);
		cp::RpcValue::MetaData md;
		m_chainpackReader->read(md);
		if(m_pathsTypeDescr.empty()) {
			m_logHeader = ShvLogHeader::fromMetaData(md);
			m_pathsTypeDescr = m_logHeader.pathsTypeDescr();
		}
		chainpack::ChainPackReader::ItemType t = m_chainpackReader->unpackNext();
		if(t != chainpack::ChainPackReader::ItemType::CCPCP_ITEM_LIST)
			SHV_EXCEPTION("Log is corrupted!");
	}
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
		using Column = ShvLogHeader::Column;
		while(true) {
			if(!m_ifstream)
				return ShvJournalEntry();
			if(m_chainpackReader->peekNext() == chainpack::ChainPackReader::ItemType::CCPCP_ITEM_CONTAINER_END)
				return ShvJournalEntry();

			cp::RpcValue val;
			m_chainpackReader->read(val);
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
			e.epochMsec = time;
			e.path = path;
			e.value = row.value(Column::Value);
			cp::RpcValue st = row.value(Column::ShortTime);
			e.shortTime = st.isInt() && st.toInt() >= 0? st.toInt(): ShvJournalEntry::NO_SHORT_TIME;
			e.domain = row.value(Column::Domain).toString();
			e.sampleType = pathsSampleType(e.path);
			break;
		}
	}
	else while(true) {
		using Column = FileShvJournal2::TxtColumn;
		if(!m_ifstream)
			return ShvJournalEntry();

		std::string line = getLine(m_ifstream, '\n');
		shv::core::StringView sv(line);
		shv::core::StringViewList line_record = sv.split('\t', shv::core::StringView::KeepEmptyParts);
		if(line_record.empty()) {
			logDShvJournal() << "skipping empty line";
			continue; // skip empty line
		}
		std::string dtstr = line_record[Column::Timestamp].toString();
		std::string path = line_record.value(Column::Path).toString();
		std::string domain = line_record.value(Column::Domain).toString();
		size_t len;
		cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromUtcString(dtstr, &len);
		if(len == 0) {
			logWShvJournal() << "invalid date time string:" << dtstr << "line will be ignored";
			continue;
		}
		StringView short_time_sv = line_record.value(Column::ShortTime);

		e.path = std::move(path);
		e.epochMsec = dt.msecsSinceEpoch();
		e.shortTime = short_time_sv.toInt();
		e.domain = std::move(domain);
		std::string err;
		e.value = cp::RpcValue::fromCpon(line_record.value(Column::Value).toString(), &err);
		if(!err.empty())
			logWShvJournal() << "Invalid CPON value:" << line_record.value(Column::Value).toString();
		e.sampleType = pathsSampleType(e.path);
		break;
	}
	return e;
}

ShvLogTypeDescr::SampleType ShvLogFileReader::pathsSampleType(const std::string &path) const
{
	auto it = m_pathsTypeDescr.find(path);
	return it == m_pathsTypeDescr.end()? ShvJournalEntry::SampleType::Continuous: it->second.sampleType;
}


} // namespace utils
} // namespace core
} // namespace shv
