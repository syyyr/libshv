#ifndef SHV_CORE_UTILS_SHVLOGFILEREADER_H
#define SHV_CORE_UTILS_SHVLOGFILEREADER_H

#include "../shvcoreglobal.h"

#include "shvlogheader.h"
#include "shvjournalentry.h"

#include <shv/chainpack/chainpackreader.h>

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
	ShvLogFileReader(const std::string &file_name, const ShvLogHeader *header = nullptr);
	~ShvLogFileReader();

	bool next();
	const ShvJournalEntry& entry();

	const ShvLogHeader &logHeader() const {return m_logHeader;}
private:
	ShvLogTypeDescr::SampleType pathsSampleType(const std::string &path) const;
private:
	ShvLogHeader m_logHeader;
	std::ifstream m_ifstream;
	shv::chainpack::ChainPackReader m_chainpackReader;
	ShvJournalEntry m_currentEntry;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGFILEREADER_H
