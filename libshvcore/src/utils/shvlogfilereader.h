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
	const ShvLogHeader &logHeader() const {return m_logHeader;}
private:
	ShvLogTypeDescr::SampleType pathsSampleType(const std::string &path) const;
private:
	std::ifstream m_ifstream;
	bool m_isTextLog = false;
	ShvLogHeader m_logHeader;
	std::map<std::string, ShvLogTypeDescr> m_pathsTypeDescr;
	shv::chainpack::ChainPackReader *m_chainpackReader = nullptr;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGFILEREADER_H
