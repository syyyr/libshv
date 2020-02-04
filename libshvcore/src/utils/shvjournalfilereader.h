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
	ShvJournalFileReader(const std::string &file_name, const ShvLogHeader *header);

	bool next();
	const ShvJournalEntry& entry();
private:
	ShvLogTypeDescr::SampleType pathsSampleType(const std::string &path) const;
private:
	std::ifstream m_ifstream;
	const ShvLogHeader *m_logHeader = nullptr;
	std::map<std::string, ShvLogTypeDescr> m_pathsTypeDescr;
	ShvJournalEntry m_currentEntry;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVJOURNALFILEREADER_H
