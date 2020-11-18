#ifndef SHV_CORE_UTILS_SHVLOGRPCVALUEREADER_H
#define SHV_CORE_UTILS_SHVLOGRPCVALUEREADER_H

#include "shvlogheader.h"
#include "shvjournalentry.h"

namespace shv {
namespace core {
namespace utils {

class ShvJournalEntry;

class SHVCORE_DECL_EXPORT ShvLogRpcValueReader
{
public:
	ShvLogRpcValueReader(const shv::chainpack::RpcValue &log, bool throw_exceptions = false);

	bool next();
	const ShvJournalEntry& entry() { return m_currentEntry; }

	const ShvLogHeader &logHeader() const {return m_logHeader;}
private:
	ShvLogHeader m_logHeader;
	ShvJournalEntry m_currentEntry;

	const shv::chainpack::RpcValue m_log;
	bool m_isThrowExceptions;
	size_t m_currentIndex = 0;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGRPCVALUEREADER_H
