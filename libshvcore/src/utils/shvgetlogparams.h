#ifndef SHV_CORE_UTILS_SHVGETLOGPARAMS_H
#define SHV_CORE_UTILS_SHVGETLOGPARAMS_H

#include "../shvcoreglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <string>

namespace shv {
namespace core {
namespace utils {

struct SHVCORE_DECL_EXPORT ShvGetLogParams
{
	static const char *KEY_HEADER_OPTIONS_DEPRECATED;
	static const char *KEY_MAX_RECORD_COUNT_DEPRECATED;
	static const char *KEY_RECORD_COUNT_LIMIT;
	static const char *KEY_WITH_SNAPSHOT;
	//static const char *KEY_WITH_UPTIME;
	static const char *KEY_WITH_PATHS_DICT;
	//static const char *KEY_WITH_TYPE_INFO;
	static const char *KEY_WITH_SINCE;
	static const char *KEY_WITH_UNTIL;
	static const char *KEY_PATH_PATTERN;
	static const char *KEY_PATH_PATTERN_TYPE;
	static const char *KEY_DOMAIN_PATTERN;

	static const char *SINCE_LAST;

	shv::chainpack::RpcValue since;
	shv::chainpack::RpcValue until;
	/// '*' and '**' wild-cards are supported
	/// '*' stands for single path segment, shv/pol/*/discon match shv/pol/ols/discon but not shv/pol/ols/depot/discon
	/// '**' stands for zero or more path segments, shv/pol/**/discon matches shv/pol/discon, shv/pol/ols/discon, shv/pol/ols/depot/discon
	/// std::regex::regex_math is checked for RegExp type
	std::string pathPattern;
	enum class PatternType {WildCard, RegEx};
	PatternType pathPatternType = PatternType::WildCard;
	//unsigned headerOptions = static_cast<unsigned>(HeaderOptions::BasicInfo);
	static constexpr int DEFAULT_RECORD_COUNT_LIMIT = 1000;
	int recordCountLimit = DEFAULT_RECORD_COUNT_LIMIT;
	bool withSnapshot = false;
	//bool withUptime = false;
	std::string domainPattern; /// always regexp
	bool withTypeInfo = false;
	bool withPathsDict = true;

	ShvGetLogParams() {}
	ShvGetLogParams(const shv::chainpack::RpcValue &opts);

	shv::chainpack::RpcValue toRpcValue(bool fill_legacy_fields = true) const;
	static ShvGetLogParams fromRpcValue(const shv::chainpack::RpcValue &v);

	bool isSinceLast() const;
};

} // namespace utils
} // namespace iotqt
} // namespace shv

#endif // SHV_IOTQT_UTILS_SHVGETLOGPARAMS_H
