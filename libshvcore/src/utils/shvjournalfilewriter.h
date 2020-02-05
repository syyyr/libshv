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
	ShvJournalFileWriter(const std::string &file_name, int64_t start_msec = 0);

	void appendMonotonic(const ShvJournalEntry &entry);
	void append(const ShvJournalEntry &entry);

	ssize_t fileSize();
private:
	void append(int64_t msec, int uptime, const ShvJournalEntry &entry);
private:
	std::string m_fileName;
	std::ofstream m_out;
	int64_t m_fileNameTimeStamp = 0;
	int64_t m_recentTimeStamp = 0;
	//ssize_t m_origFileSize = 0;
	//ssize_t m_newFileSize = 0;

};

} // namespace utils
} // namespace core
} // namespace shv

