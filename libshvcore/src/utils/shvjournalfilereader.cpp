#include "shvjournalfilereader.h"
#include "shvfilejournal.h"
#include "shvlogheader.h"

#include "../exception.h"
#include "../log.h"
#include "../stringview.h"

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logMShvJournal() shvCMessage("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

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

ShvJournalFileReader::ShvJournalFileReader(const std::string &file_name, const ShvLogHeader *header)
	: m_fileName(file_name)
	, m_logHeader(header)
{
	m_ifstream.open(file_name, std::ios::binary);
	if(!m_ifstream)
		SHV_EXCEPTION("Cannot open file " + file_name + " for reading.");

	if(header)
		m_pathsTypeDescr = m_logHeader->pathsTypeDescr();
	else
		shvWarning() << "Type info not provided for text journal, sample type cannot be discovered.";
}

ShvLogTypeDescr::SampleType ShvJournalFileReader::pathsSampleType(const std::string &path) const
{
	auto it = m_pathsTypeDescr.find(path);
	return it == m_pathsTypeDescr.end()? ShvJournalEntry::SampleType::Continuous: it->second.sampleType;
}

bool ShvJournalFileReader::next()
{
	using Column = ShvFileJournal::TxtColumn;
	while(true) {
		m_currentEntry = ShvJournalEntry();
		if(!m_ifstream)
			return false;

		std::string line = getLine(m_ifstream, ShvFileJournal::RECORD_SEPARATOR);
		shv::core::StringView sv(line);
		shv::core::StringViewList line_record = sv.split(ShvFileJournal::FIELD_SEPARATOR, shv::core::StringView::KeepEmptyParts);
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

		m_currentEntry.path = std::move(path);
		m_currentEntry.epochMsec = dt.msecsSinceEpoch();
		bool ok;
		int short_time = short_time_sv.toInt(&ok);
		m_currentEntry.shortTime = ok && short_time >= 0? short_time: ShvJournalEntry::NO_SHORT_TIME;
		m_currentEntry.domain = std::move(domain);
		std::string err;
		m_currentEntry.value = cp::RpcValue::fromCpon(line_record.value(Column::Value).toString(), &err);
		if(!err.empty())
			logWShvJournal() << "Invalid CPON value:" << line_record.value(Column::Value).toString();
		m_currentEntry.sampleType = pathsSampleType(m_currentEntry.path);
		return true;
	}
}

bool ShvJournalFileReader::last()
{
	ssize_t fpos;
	ShvFileJournal::findLastEntryDateTime(m_fileName, &fpos);
	if(fpos >= 0) {
		m_ifstream.clear();
		m_ifstream.seekg(fpos, std::ios::beg);
		return next();
	}
	else {
		m_currentEntry = ShvJournalEntry();
		return false;
	}
}

const ShvJournalEntry &ShvJournalFileReader::entry()
{
	return m_currentEntry;
}

} // namespace utils
} // namespace core
} // namespace shv
