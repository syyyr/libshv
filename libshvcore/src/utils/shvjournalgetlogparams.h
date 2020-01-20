#ifndef SHV_CORE_UTILS_SHVJOURNALGETLOGPARAMS_H
#define SHV_CORE_UTILS_SHVJOURNALGETLOGPARAMS_H

#include "../shvcoreglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <string>

namespace shv {
namespace core {
namespace utils {

struct SHVCORE_DECL_EXPORT ShvJournalGetLogParams
{
	static const char *KEY_HEADER_OPTIONS;
	static const char *KEY_MAX_RECORD_COUNT;
	static const char *KEY_WITH_SNAPSHOT;
	static const char *KEY_WITH_UPTIME;
	static const char *KEY_WITH_SINCE;
	static const char *KEY_WITH_UNTIL;
	static const char *KEY_PATH_PATTERN;
	static const char *KEY_PATH_PATTERN_TYPE;
	static const char *KEY_DOMAIN_PATTERN;

	shv::chainpack::RpcValue since;
	shv::chainpack::RpcValue until;
	/// '*' and '**' wild-cards are supported
	/// '*' stands for single path segment, shv/pol/*/discon match shv/pol/ols/discon but not shv/pol/ols/depot/discon
	/// '**' stands for zero or more path segments, shv/pol/**/discon matches shv/pol/discon, shv/pol/ols/discon, shv/pol/ols/depot/discon
	/// std::regex::regex_math is checked for RegExp type
	std::string pathPattern;
	enum class PatternType {WildCard, RegEx};
	PatternType pathPatternType = PatternType::WildCard;

	enum class HeaderOptions : unsigned {
		BasicInfo = 1 << 0,
		FieldInfo = 1 << 1,
		TypeInfo = 1 << 2,
		PathsDict = 1 << 3,
		CompleteInfo = BasicInfo | FieldInfo | TypeInfo | PathsDict,
	};
	unsigned headerOptions = static_cast<unsigned>(HeaderOptions::BasicInfo);
	static constexpr int DEFAULT_RECORD_COUNT_MAX = 1000;
	int maxRecordCount = DEFAULT_RECORD_COUNT_MAX;
	bool withSnapshot = false;
	bool withUptime = false;
	std::string domainPattern; /// always regexp
	ShvJournalGetLogParams() {}
	ShvJournalGetLogParams(const shv::chainpack::RpcValue &opts);

	shv::chainpack::RpcValue toRpcValue() const;
};

} // namespace utils
} // namespace iotqt
} // namespace shv

#endif // SHV_IOTQT_UTILS_SHVJOURNALGETLOGPARAMS_H
