#include "shvjournalfilereader.h"
#include "shvfilejournal.h"
#include "shvlogheader.h"

#include "../exception.h"
#include "../log.h"
#include "../stringview.h"
#include "../string.h"

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
		auto c = in.get();
		if(c == std::char_traits<char>::eof())
			return line;
		if(c == sep)
			return line;
		if(c == 0)
			continue; // sometimes log file contains zeros, skip them, istream::getline cannot handle this
		line += std::char_traits<char>::to_char_type(c);
	}
	return line;
}

ShvJournalFileReader::ShvJournalFileReader(const std::string &file_name)
	: m_fileName(file_name)
{
	m_ifstream.open(file_name, std::ios::binary);
	if(!m_ifstream)
		SHV_EXCEPTION("Cannot open file " + file_name + " for reading.");
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
		size_t len;
		cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromUtcString(dtstr, &len);
		//logDShvJournal() << dtstr << "-->" << dt.toIsoString();
		if(len == 0) {
			logWShvJournal() << "invalid date time string:" << dtstr << "line will be ignored";
			continue;
		}
		if(len >= line.size() || line[len] != ShvFileJournal::FIELD_SEPARATOR) {
			logWShvJournal() << "invalid date time string:" << dtstr << "correct date time should end with field separator on position:" << len << ", line will be ignored";
			continue;
		}
		std::string path = line_record.value(Column::Path).toString();
		std::string domain = line_record.value(Column::Domain).toString();
		StringView short_time_sv = line_record.value(Column::ShortTime);
		auto sample_type = static_cast<ShvJournalEntry::SampleType>(shv::core::String::toInt(line_record.value(Column::SampleType).toString()));

		m_currentEntry.path = std::move(path);
		m_currentEntry.epochMsec = dt.msecsSinceEpoch();
		if(m_snapShotEpochMsec == 0)
			m_snapShotEpochMsec = m_currentEntry.epochMsec;
		bool ok;
		int short_time = short_time_sv.toInt(&ok);
		m_currentEntry.shortTime = ok && short_time >= 0? short_time: ShvJournalEntry::NO_SHORT_TIME;
		m_currentEntry.domain = std::move(domain);
		m_currentEntry.sampleType = sample_type;
		if (m_currentEntry.sampleType == ShvJournalEntry::SampleType::Invalid)
			m_currentEntry.sampleType = ShvJournalEntry::SampleType::Continuous;
		m_currentEntry.userId = line_record.value(Column::UserId).toString();
		std::string err;
		m_currentEntry.value = cp::RpcValue::fromCpon(line_record.value(Column::Value).toString(), &err);
		if(!err.empty())
			logWShvJournal() << "Invalid CPON value:" << line_record.value(Column::Value).toString();
		return true;
	}
}

bool ShvJournalFileReader::last()
{
	ssize_t fpos;
	ShvFileJournal::findLastEntryDateTime(m_fileName, 0, &fpos);
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
