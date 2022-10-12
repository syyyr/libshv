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
	m_snapshotMsec = fileNameToFileMsec(file_name, !shv::core::Exception::Throw);
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
		if(path.empty()) {
			logWShvJournal() << "skipping invalid line with empy path, line:" << line;
			continue;
		}
		std::string domain = line_record.value(Column::Domain).toString();
		if(domain.empty())
			domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
		StringView short_time_sv = line_record.value(Column::ShortTime);
		auto value_flags = shv::core::String::toInt(line_record.value(Column::ValueFlags).toString());

		m_currentEntry.path = std::move(path);
		m_currentEntry.epochMsec = dt.msecsSinceEpoch();
		bool ok;
		int short_time = short_time_sv.toInt(&ok);
		m_currentEntry.shortTime = ok && short_time >= 0? short_time: ShvJournalEntry::NO_SHORT_TIME;
		m_currentEntry.domain = std::move(domain);
		m_currentEntry.valueFlags = static_cast<unsigned int>(value_flags);
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

bool ShvJournalFileReader::inSnapshot()
{
	if(m_inSnapshot) {
		m_inSnapshot = (m_currentEntry.epochMsec == m_snapshotMsec) && m_currentEntry.isSnapshotValue();
	}
	return m_inSnapshot;
}

static constexpr size_t MIN_SEP_POS = 13;
static constexpr size_t SEC_SEP_POS = MIN_SEP_POS + 3;
static constexpr size_t MSEC_SEP_POS = SEC_SEP_POS + 3;

int64_t ShvJournalFileReader::fileNameToFileMsec(const std::string &fn, bool throw_exc)
{
	std::string utc_str;
	auto ix = fn.rfind('/');
	if(ix != std::string::npos)
		utc_str = fn.substr(ix + 1);
	else
		utc_str = fn;
	if(MSEC_SEP_POS >= utc_str.size()) {
		if(throw_exc)
			SHV_EXCEPTION("fileNameToFileMsec(): File name: '" + fn + "' too short.");
		else
			return -1;
	}
	utc_str[MIN_SEP_POS] = ':';
	utc_str[SEC_SEP_POS] = ':';
	utc_str[MSEC_SEP_POS] = '.';
	size_t len;
	int64_t msec = cp::RpcValue::DateTime::fromUtcString(utc_str, &len).msecsSinceEpoch();
	if(msec == 0 || len == 0) {
		if(throw_exc)
			SHV_EXCEPTION("fileNameToFileMsec(): Invalid file name: '" + fn + "' cannot be converted to date time");
		else
			return -1;
	}
	return msec;
}

std::string ShvJournalFileReader::msecToBaseFileName(int64_t msec)
{
	std::string fn = cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString(cp::RpcValue::DateTime::MsecPolicy::Always, false);
	fn[MIN_SEP_POS] = '-';
	fn[SEC_SEP_POS] = '-';
	fn[MSEC_SEP_POS] = '-';
	return fn;
}

} // namespace utils
} // namespace core
} // namespace shv
