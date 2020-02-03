#ifndef SHV_CORE_UTILS_SHVDIRTYLOG_H
#define SHV_CORE_UTILS_SHVDIRTYLOG_H

#include "../shvcoreglobal.h"

#include <string>

namespace shv {
namespace core {
namespace utils {

class ShvJournalEntry;

class SHVCORE_DECL_EXPORT ShvDirtyLog
{
public:
	ShvDirtyLog(const std::string &file_name);

	void append(const ShvJournalEntry &entry);
private:
	std::string m_fileName;
	int64_t m_recentTimeStamp = 0;

};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVDIRTYLOG_H
