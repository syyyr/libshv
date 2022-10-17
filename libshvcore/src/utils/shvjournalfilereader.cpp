#include "shvjournalfilereader.h"
#include "shvfilejournal.h"

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
		if(line.empty()) {
			logDShvJournal() << "skipping empty line";
			continue; // skip empty line
		}
		std::string::size_type ix1 = 0;
		for(int column = 0; column < ShvFileJournal::TxtColumnCount && ix1 != std::string::npos; ++column) {
			auto ix2 = line.find(ShvFileJournal::FIELD_SEPARATOR, ix1);
			StringView fld;
			if(ix2 == std::string::npos) {
				fld = StringView(line, ix1);
				ix1 = ix2;
			}
			else {
				fld = StringView(line, ix1, ix2 - ix1);
				ix1 = ix2 + 1;
			}
			switch(column) {
			case Column::Timestamp: {
				std::string dtstr = fld.toString();
				size_t len;
				cp::RpcValue::DateTime dt = cp::RpcValue::DateTime::fromUtcString(dtstr, &len);
				//logDShvJournal() << dtstr << "-->" << dt.toIsoString();
				if(len == 0) {
					logWShvJournal() << "invalid date time string:" << dtstr << "line will be ignored";
					goto next_line;
				}
				if(len >= line.size() || line[len] != ShvFileJournal::FIELD_SEPARATOR) {
					logWShvJournal() << "invalid date time string:" << dtstr << "correct date time should end with field separator on position:" << len << ", line will be ignored";
					goto next_line;
				}
				m_currentEntry.epochMsec = dt.msecsSinceEpoch();
				break;
			}
			case Column::Path: {
				if(fld.empty()) {
					logWShvJournal() << "skipping invalid line with empy path, line:" << line;
					goto next_line;
				}
				m_currentEntry.path = fld.toString();
				break;
			}
			case Column::Value: {
				std::string err;
				m_currentEntry.value = cp::RpcValue::fromCpon(fld.toString(), &err);
				if(!err.empty()) {
					logWShvJournal() << "Invalid CPON value:" << fld.toString();
					goto next_line;
				}
				break;
			}
			case Column::Domain: {
				if(fld.empty())
					m_currentEntry.domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
				else
					m_currentEntry.domain = fld.toString();
				break;
			}
			case Column::ShortTime: {
				if(fld.empty()) {
					m_currentEntry.shortTime = ShvJournalEntry::NO_SHORT_TIME;
				}
				else {
					bool ok;
					int short_time = fld.toInt(&ok);
					m_currentEntry.shortTime = ok && short_time >= 0? short_time: ShvJournalEntry::NO_SHORT_TIME;
				}
				break;
			}
			case Column::ValueFlags: {
				auto value_flags = fld.empty()? 0: fld.toInt();
				m_currentEntry.valueFlags = static_cast<unsigned int>(value_flags);
				break;
			}
			case Column::UserId: {
				m_currentEntry.userId = fld.toString();
				break;
			}
			}
		}
		return true;
next_line:
		m_currentEntry = {};
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
