#include "shvjournalfilewriter.h"
#include "shvfilejournal.h"
#include "../exception.h"

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

static int uptimeSec()
{
	int uptime;
	if (std::ifstream("/proc/uptime", std::ios::in) >> uptime) {
		return uptime;
	}
	return 0;
}

ShvJournalFileWriter::ShvJournalFileWriter(const std::string &file_name)
	: m_fileName(file_name)
{
	open();
}

ShvJournalFileWriter::ShvJournalFileWriter(const std::string &journal_dir, int64_t journal_start_time, int64_t last_entry_ts)
	: m_fileName(journal_dir + '/' + ShvFileJournal::JournalContext::fileMsecToFileName(journal_start_time))
	, m_recentTimeStamp(last_entry_ts)
{
	open();
}

void ShvJournalFileWriter::open()
{
	m_out.open(m_fileName, std::ios::binary | std::ios::out | std::ios::app);
	if(!m_out)
		SHV_EXCEPTION("Cannot open file " + m_fileName + " for writing");
}

ssize_t ShvJournalFileWriter::fileSize()
{
	return m_out.tellp();
}

void ShvJournalFileWriter::appendMonotonic(const ShvJournalEntry &entry)
{

	ssize_t fsz = fileSize();
	if(m_recentTimeStamp == 0) {
		if(fsz == 0)
			m_recentTimeStamp = ShvFileJournal::JournalContext::fileNameToFileMsec(m_fileName);
		else
			m_recentTimeStamp = ShvFileJournal::findLastEntryDateTime(m_fileName);
	}
	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	if(msec < m_recentTimeStamp)
		msec = m_recentTimeStamp;
	append(msec, uptimeSec(), entry);
}

void ShvJournalFileWriter::append(const ShvJournalEntry &entry)
{
	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	append(msec, 0, entry);
}

void ShvJournalFileWriter::append(int64_t msec, int uptime, const ShvJournalEntry &entry)
{
	m_out << cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString();
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	m_out << uptime;
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	m_out << entry.path;
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	m_out << entry.value.toCpon();
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	if(entry.shortTime >= 0)
		m_out << entry.shortTime;
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	m_out << entry.domain;
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	if(entry.sampleType != ShvJournalEntry::SampleType::Invalid)
		m_out << (int)entry.sampleType;
	m_out << ShvFileJournal::RECORD_SEPARATOR;
	m_out.flush();
	m_recentTimeStamp = msec;
}

} // namespace utils
} // namespace core
} // namespace shv
