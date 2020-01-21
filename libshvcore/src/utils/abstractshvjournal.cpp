#include "abstractshvjournal.h"
#include "shvjournalentry.h"
#include "shvjournalgetlogparams.h"
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

AbstractShvJournal::PatternMatcher::PatternMatcher(const ShvJournalGetLogParams &filter)
{
	if(!filter.pathPattern.empty()) {
		if(filter.pathPatternType == ShvJournalGetLogParams::PatternType::RegEx) {
			try {
				m_pathPatternRegEx = std::regex(filter.pathPattern);
				m_usePathPatternregEx = true;
			}
			catch (const std::regex_error &e) {
				m_regexError = true;
				logWShvJournal() << "Invalid path pattern regex:" << filter.pathPattern << " - " << e.what();
			}
		}
		else {
			m_pathPatternWildCard = filter.pathPattern;
		}
	}
	if(!filter.domainPattern.empty()) try {
		m_domainPatternRegEx = std::regex(filter.domainPattern);
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
	if(isEmpty())
		return true;
	if(m_usePathPatternregEx) {
		return std::regex_match(path, m_pathPatternRegEx);
	}
	else if(!m_pathPatternWildCard.empty()) {
		const shv::core::StringViewList path_lst = shv::core::StringView(path).split('/');
		const shv::core::StringViewList pattern_lst = shv::core::StringView(m_pathPatternWildCard).split('/');
		return ShvPath::matchWild(path_lst, pattern_lst);
	}
	if(m_useDomainPatternregEx) {
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
	case Column::Enum::Course: return "course";
	}
	return "invalid";
}

} // namespace utils
} // namespace core
} // namespace shv
