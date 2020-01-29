#ifndef SHV_CORE_UTILS_SHVLOGFILEREADER_H
#define SHV_CORE_UTILS_SHVLOGFILEREADER_H

#include "../shvcoreglobal.h"

#include "shvlogheader.h"

#include <string>
#include <fstream>

namespace shv {
namespace chainpack { class ChainPackReader; }
namespace core {
namespace utils {

class ShvJournalEntry;

class SHVCORE_DECL_EXPORT ShvLogFileReader
{
public:
	ShvLogFileReader(const std::string &file_name);
	ShvLogFileReader(const std::string &file_name, const ShvLogHeader &header);
	~ShvLogFileReader();

	ShvJournalEntry next();
private:
	ShvLogTypeDescription::SampleType pathsSampleType(const std::string &path) const;
private:
	std::ifstream m_ifstream;
	bool m_isTextLog = false;
	ShvLogHeader m_logHeader;
	std::map<std::string, ShvLogTypeDescription::SampleType> m_pathsSampleTypes;
	shv::chainpack::ChainPackReader *m_chainpackReader = nullptr;

	size_t m_colTimestamp = 0;
	size_t m_colPath = 0;
	size_t m_colValue = 0;
	size_t m_colShortTime = 0;
	size_t m_colDomain = 0;};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGFILEREADER_H
