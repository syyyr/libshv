#ifndef SHV_CORE_UTILS_SHVJOURNALCOMMON_H
#define SHV_CORE_UTILS_SHVJOURNALCOMMON_H


namespace shv {
namespace core {
namespace utils {

class ShvJournalCommon
{
public:
	static constexpr long DEFAULT_FILE_SIZE_LIMIT = 100 * 1024;
	static constexpr int DEFAULT_GET_LOG_RECORD_COUNT_LIMIT = 100 * 1000;

	static const char *KEY_NAME;
	static const char *KEY_RECORD_COUNT;
	static const char *KEY_PATHS_DICT;

	struct Column
	{
		enum Enum {
			Timestamp = 0,
			UpTime,
			Path,
			Value,
			ShortTime,
			Domain,
			Course,
		};
		static const char* name(Enum e);
	};
protected:
	int m_getLogRecordCountLimit = DEFAULT_GET_LOG_RECORD_COUNT_LIMIT;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVJOURNALCOMMON_H
