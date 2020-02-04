#include "shvjournalfilewriter.h"
#include "shvfilejournal.h"

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

static int uptimeSec()
{
	/*
	int uptime;
	if (std::ifstream("/proc/uptime", std::ios::in) >> uptime) {
		return uptime;
	}
	*/
	return 0;
}

ShvJournalFileWriter::ShvJournalFileWriter(const std::string &file_name, int64_t start_msec)
	: m_fileName(file_name)
	, m_fileNameTimeStamp(start_msec)
{
	m_out.open(m_fileName, std::ios::binary | std::ios::out | std::ios::app);
	if(!m_out)
		throw std::runtime_error("Cannot open file " + m_fileName + " for writing");
}

ssize_t ShvJournalFileWriter::fileSize()
{
	return m_out.tellp();
}

void ShvJournalFileWriter::appendMonotonic(const ShvJournalEntry &entry)
{

	ssize_t fsz = fileSize();
	if(fsz == 0) {
		if(m_fileNameTimeStamp == 0)
			throw std::runtime_error("appendMonotonic() to file " + m_fileName + " must have start timestamp defined!");
		m_recentTimeStamp = m_fileNameTimeStamp;
	}
	else if(fsz > 0 && m_recentTimeStamp == 0) {
		m_recentTimeStamp = ShvFileJournal::findLastEntryDateTime(m_fileName);
	}
	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	if(msec < m_recentTimeStamp)
		msec = m_recentTimeStamp;
	append(msec, entry);
}

void ShvJournalFileWriter::append(const ShvJournalEntry &entry)
{
	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	append(msec, entry);
}

void ShvJournalFileWriter::append(int64_t msec, const ShvJournalEntry &entry)
{
	m_out << cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString();
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	m_out << uptimeSec();
	//m_out << msec;
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	m_out << entry.path;
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	m_out << entry.value.toCpon();
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	if(entry.shortTime >= 0)
		m_out << entry.shortTime;
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	m_out << entry.domain;
	m_out << ShvFileJournal::RECORD_SEPARATOR;
	m_out.flush();
	m_recentTimeStamp = msec;
}

} // namespace utils
} // namespace core
} // namespace shv
