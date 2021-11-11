#include "patternmatcher.h"

#include "../log.h"
#include "../stringview.h"
#include "shvpath.h"

#define logWShvJournal() shvCWarning("ShvJournal")

namespace shv {
namespace core {
namespace utils {

PatternMatcher::PatternMatcher(const ShvGetLogParams &filter)
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

bool PatternMatcher::match(const ShvJournalEntry &entry) const
{
	return match(entry.path, entry.domain);
}

bool PatternMatcher::match(const std::string &path, const std::string &domain) const
{
	//shvLogFuncFrame() << "path:" << path << "domain:" << domain;
	if(isEmpty()) {
		//shvDebug() << "empty filter matches ALL";
		return true;
	}
	if(m_usePathPatternRegEx) {
		//shvDebug() << "using path pattern regex";
		std::smatch cmatch;
		if(!std::regex_search(path, cmatch, m_pathPatternRegEx))
			return false;
	}
	else if(!m_pathPatternWildCard.empty()) {
		//shvDebug() << "using path pattern wildcard:" << m_pathPatternWildCard;
		const shv::core::StringViewList path_lst = shv::core::utils::ShvPath::split(path);
		const shv::core::StringViewList pattern_lst = shv::core::StringView(m_pathPatternWildCard).split('/');
		if(!ShvPath::matchWild(path_lst, pattern_lst))
			return false;
	}
	if(m_useDomainPatternregEx) {
		//shvDebug() << "using domain pattern regex";
		return std::regex_match(domain, m_domainPatternRegEx);
	}
	return true;
}

}
}
}

