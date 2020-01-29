#include "abstractshvjournal.h"
#include "shvjournalentry.h"
#include "shvgetlogparams.h"
#include "shvpath.h"
#include "../stringview.h"

#include "../log.h"

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace shv {
namespace core {
namespace utils {

const int AbstractShvJournal::DEFAULT_GET_LOG_RECORD_COUNT_LIMIT = 100 * 1000;

AbstractShvJournal::PatternMatcher::PatternMatcher(const ShvGetLogParams &filter)
{
	shvLogFuncFrame() << "params:" << filter.toRpcValue().toCpon();
	if(!filter.pathPattern.empty()) {
		shvDebug() << "path pattern:" << filter.pathPattern;
		if(filter.pathPatternType == ShvGetLogParams::PatternType::RegEx) {
			shvDebug() << "\t regex";
			try {
				m_pathPatternRegEx = std::regex(filter.pathPattern);
				shvDebug() << "\t\t OK";
				m_usePathPatternRegEx = true;
			}
			catch (const std::regex_error &e) {
				m_regexError = true;
				logWShvJournal() << "Invalid path pattern regex:" << filter.pathPattern << " - " << e.what();
			}
		}
		else {
			shvDebug() << "\t wildcard";
			m_pathPatternWildCard = filter.pathPattern;
			shvDebug() << "\t\t OK";
		}
	}
	if(!filter.domainPattern.empty()) try {
		shvDebug() << "domain pattern:" << filter.domainPattern;
		shvDebug() << "\t regex";
		m_domainPatternRegEx = std::regex(filter.domainPattern);
		shvDebug() << "\t\t OK";
		m_useDomainPatternregEx = true;
	}
	catch (const std::regex_error &e) {
		m_regexError = true;
		logWShvJournal() << "Invalid domain pattern regex:" << filter.domainPattern << " - " << e.what();
	}
}

bool AbstractShvJournal::PatternMatcher::match(const ShvJournalEntry &entry) const
{
	return match(entry.path, entry.domain);
}

bool AbstractShvJournal::PatternMatcher::match(const std::string &path, const std::string &domain) const
{
	shvLogFuncFrame() << "path:" << path << "domain:" << domain;
	if(isEmpty()) {
		shvDebug() << "empty filter matches ALL";
		return true;
	}
	if(m_usePathPatternRegEx) {
		shvDebug() << "using path pattern regex";
		if(!std::regex_match(path, m_pathPatternRegEx))
			return false;
	}
	else if(!m_pathPatternWildCard.empty()) {
		shvDebug() << "using path pattern wildcard:" << m_pathPatternWildCard;
		const shv::core::StringViewList path_lst = shv::core::StringView(path).split('/');
		const shv::core::StringViewList pattern_lst = shv::core::StringView(m_pathPatternWildCard).split('/');
		if(!ShvPath::matchWild(path_lst, pattern_lst))
			return false;
	}
	if(m_useDomainPatternregEx) {
		shvDebug() << "using domain pattern regex";
		return std::regex_match(domain, m_domainPatternRegEx);
	}
	return true;
}

const char *AbstractShvJournal::KEY_NAME = "name";
const char *AbstractShvJournal::KEY_RECORD_COUNT = "recordCount";
const char *AbstractShvJournal::KEY_PATHS_DICT = "pathsDict";

AbstractShvJournal::~AbstractShvJournal()
{
}

const char *AbstractShvJournal::Column::name(AbstractShvJournal::Column::Enum e)
{
	switch (e) {
	case Column::Enum::Timestamp: return "timestamp";
	case Column::Enum::UpTime: return "upTime";
	case Column::Enum::Path: return "path";
	case Column::Enum::Value: return "value";
	case Column::Enum::ShortTime: return "shortTime";
	case Column::Enum::Domain: return "domain";
	}
	return "invalid";
}

} // namespace utils
} // namespace core
} // namespace shv
