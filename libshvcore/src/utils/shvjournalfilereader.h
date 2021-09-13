#ifndef SHV_CORE_UTILS_SHVJOURNALFILEREADER_H
#define SHV_CORE_UTILS_SHVJOURNALFILEREADER_H

#include "../shvcoreglobal.h"
#include "shvjournalentry.h"
#include "shvlogtypeinfo.h"

#include <string>
#include <fstream>

namespace shv {
namespace core {
namespace utils {

class ShvLogHeader;

class SHVCORE_DECL_EXPORT ShvJournalFileReader
{
public:
	ShvJournalFileReader(const std::string &file_name);

	bool next();
	bool last();
	const ShvJournalEntry& entry();
private:
	std::string m_fileName;
	std::ifstream m_ifstream;
	ShvJournalEntry m_currentEntry;
	int64_t m_snapshotMsec = -1;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVJOURNALFILEREADER_H
