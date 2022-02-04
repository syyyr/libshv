#include "shvjournalfilewriter.h"
#include "shvfilejournal.h"
#include "../exception.h"
#include "../log.h"

#include <shv/chainpack/rpc.h>

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
	//if(m_recentTimeStamp <= 0)
	//	SHV_EXCEPTION("Cannot append to file " + m_fileName + ", find recent entry timestamp error.");
}

ssize_t ShvJournalFileWriter::fileSize()
{
	return m_out.tellp();
}

void ShvJournalFileWriter::append(const ShvJournalEntry &entry)
{
	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	m_recentTimeStamp = msec;
	append(msec, uptimeSec(), entry);
}

void ShvJournalFileWriter::appendMonotonic(const ShvJournalEntry &entry)
{

	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	if(m_recentTimeStamp > 0) {
		if(msec < m_recentTimeStamp)
			msec = m_recentTimeStamp;
	}
	else {
		m_recentTimeStamp = msec;
	}
	append(msec, uptimeSec(), entry);
}

void ShvJournalFileWriter::appendSnapshot(int64_t msec, const std::vector<ShvJournalEntry> &snapshot)
{
	int uptime = uptimeSec();
	for(ShvJournalEntry e : snapshot) {
		e.setSnapshotValue(true);
		// erase EVENT flag in the snapshot values,
		// they can trigger events during reply otherwise
		e.setSpontaneous(false);
		append(msec, uptime, e);
	}
	m_recentTimeStamp = msec;
}

void ShvJournalFileWriter::appendSnapshot(int64_t msec, const std::map<std::string, ShvJournalEntry> &snapshot)
{
	int uptime = uptimeSec();
	for(const auto &kv : snapshot) {
		ShvJournalEntry e = kv.second;
		e.setSnapshotValue(true);
		// erase EVENT flag in the snapshot values,
		// they can trigger events during reply otherwise
		e.setSpontaneous(false);
		append(msec, uptime, e);
	}
	m_recentTimeStamp = msec;
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
	m_out << (int)entry.valueFlags;
	m_out << ShvFileJournal::FIELD_SEPARATOR;
	m_out << entry.userId;
	m_out << ShvFileJournal::RECORD_SEPARATOR;
	m_out.flush();
	m_recentTimeStamp = msec;
}

} // namespace utils
} // namespace core
} // namespace shv
