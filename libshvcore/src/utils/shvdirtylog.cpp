#include "shvdirtylog.h"
#include "shvfilejournal.h"

#include <fstream>

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

ShvDirtyLog::ShvDirtyLog(const std::string &file_name)
	: m_fileName(file_name)
{
}

void ShvDirtyLog::append(const ShvJournalEntry &entry)
{
	std::ofstream out(m_fileName, std::ios::binary | std::ios::out | std::ios::app);
	if(!out)
		throw std::runtime_error("Cannot open file " + m_fileName + " for writing");

	long fsz = out.tellp();
	if(fsz > 0 && m_recentTimeStamp == 0)
		m_recentTimeStamp = ShvFileJournal::findLastEntryDateTime(m_fileName);

	int64_t msec = entry.epochMsec;
	if(msec == 0)
		msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	//if(msec < m_recentTimeStamp)
	//	msec = m_recentTimeStamp;
	out << cp::RpcValue::DateTime::fromMSecsSinceEpoch(msec).toIsoString();
	out << ShvFileJournal::FIELD_SEPARATOR;
	out << 0; // uptime is not used anymore
	out << ShvFileJournal::FIELD_SEPARATOR;
	out << entry.path;
	out << ShvFileJournal::FIELD_SEPARATOR;
	out << entry.value.toCpon();
	out << ShvFileJournal::FIELD_SEPARATOR;
	if(entry.shortTime >= 0)
		out << entry.shortTime;
	out << ShvFileJournal::FIELD_SEPARATOR;
	out << entry.domain;
	out << ShvFileJournal::RECORD_SEPARATOR;
	out.flush();
	m_recentTimeStamp = msec;
	//ssize_t file_size = out.tellp();
}

} // namespace utils
} // namespace core
} // namespace shv
