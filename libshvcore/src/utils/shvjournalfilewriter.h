#pragma once

#include "../shvcoreglobal.h"

#include <string>
#include <vector>
#include <map>
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

	void append(const ShvJournalEntry &entry);
	void appendMonotonic(const ShvJournalEntry &entry);
	void appendSnapshot(int64_t msec, const std::vector<ShvJournalEntry> &snapshot);
	void appendSnapshot(int64_t msec, const std::map<std::string, ShvJournalEntry> &snapshot);

	ssize_t fileSize();
	const std::string& fileName() const { return m_fileName; }
	int64_t recentTimeStamp() const { return m_recentTimeStamp; }
private:
	void open();
	void append(int64_t msec, int64_t orig_time, const ShvJournalEntry &entry);
private:
	std::string m_fileName;
	std::ofstream m_out;
	int64_t m_recentTimeStamp = 0;
};

} // namespace utils
} // namespace core
} // namespace shv

