#ifndef SHV_CORE_UTILS_SHVJOURNALCOMMON_H
#define SHV_CORE_UTILS_SHVJOURNALCOMMON_H

#include "../shvcoreglobal.h"

#include <string>
#include <regex>

namespace shv {
namespace chainpack { class RpcValue; }
namespace core {
namespace utils {

class ShvJournalEntry;
class ShvJournalGetLogParams;

class SHVCORE_DECL_EXPORT AbstractShvJournal
{
public:
	static constexpr long DEFAULT_FILE_SIZE_LIMIT = 100 * 1024;
	static const int DEFAULT_GET_LOG_RECORD_COUNT_LIMIT;

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
public:
	virtual ~AbstractShvJournal();

	virtual void append(const ShvJournalEntry &entry) = 0;
	virtual shv::chainpack::RpcValue getLog(const ShvJournalGetLogParams &params) = 0;
protected:
	class PatternMatcher
	{
	public:
		PatternMatcher() {}
		PatternMatcher(const ShvJournalGetLogParams &filter);
		bool isEmpty() const {return !isRegexError() && !m_usePathPatternregEx && !m_useDomainPatternregEx;}
		bool isRegexError() const {return  m_regexError;}
		bool match(const ShvJournalEntry &entry) const;
		bool match(const std::string &path, const std::string &domain) const;
	private:
		std::regex m_pathPatternRegEx;
		bool m_usePathPatternregEx = false;
		std::string m_pathPatternWildCard;

		std::regex m_domainPatternRegEx;
		bool m_useDomainPatternregEx = false;

		bool m_regexError = false;
	};
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVJOURNALCOMMON_H
