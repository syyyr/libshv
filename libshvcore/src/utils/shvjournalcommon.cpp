#include "shvjournalcommon.h"

namespace shv {
namespace core {
namespace utils {

const char *ShvJournalCommon::KEY_NAME = "name";
const char *ShvJournalCommon::KEY_RECORD_COUNT = "recordCount";
const char *ShvJournalCommon::KEY_PATHS_DICT = "pathsDict";

const char *ShvJournalCommon::Column::name(ShvJournalCommon::Column::Enum e)
{
	switch (e) {
	case Column::Enum::Timestamp: return "timestamp";
	case Column::Enum::UpTime: return "upTime";
	case Column::Enum::Path: return "path";
	case Column::Enum::Value: return "value";
	case Column::Enum::ShortTime: return "shortTime";
	case Column::Enum::Domain: return "domain";
	case Column::Enum::Course: return "course";
	}
	return "invalid";
}

} // namespace utils
} // namespace core
} // namespace shv
