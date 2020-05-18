#pragma once

#include "../shvcoreglobal.h"

#include <string>
#include <fstream>

namespace shv {
namespace core {
namespace utils {

class ShvJournalEntry;

class SHVCORE_DECL_EXPORT ShvJournalFileWriter
{
public:
	ShvJournalFileWriter(const std::string &file_name);
	ShvJournalFileWriter(const std::string &journal_dir, int64_t journal_start_time, int64_t last_entry_ts);

	void appendMonotonic(const ShvJournalEntry &entry);
	void append(const ShvJournalEntry &entry);

	ssize_t fileSize();
	const std::string& fileName() const { return m_fileName; }
	int64_t recentTimeStamp() const { return m_recentTimeStamp; }
private:
	void open();
	void append(int64_t msec, int uptime, const ShvJournalEntry &entry);
private:
	std::string m_fileName;
	std::ofstream m_out;
	int64_t m_recentTimeStamp = 0;
};

} // namespace utils
} // namespace core
} // namespace shv

