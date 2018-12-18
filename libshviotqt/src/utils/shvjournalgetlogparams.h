#ifndef SHV_IOTQT_UTILS_SHVJOURNALGETLOGPARAMS_H
#define SHV_IOTQT_UTILS_SHVJOURNALGETLOGPARAMS_H

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <string>

namespace shv {
namespace iotqt {
namespace utils {

struct SHVIOTQT_DECL_EXPORT ShvJournalGetLogParams
{
	shv::chainpack::RpcValue::DateTime since;
	shv::chainpack::RpcValue::DateTime until;
	/// '*' and '**' wild-cards are supported
	/// '*' stands for single path segment, shv/pol/*/discon match shv/pol/ols/discon but not shv/pol/ols/depot/discon
	/// '**' stands for zero or more path segments, shv/pol/**/discon matches shv/pol/discon, shv/pol/ols/discon, shv/pol/ols/depot/discon
	std::string pathPattern;
	//enum class PatternType {None = 0, WildCard, RegExp};
	//PatternType patternType = PatternType::WildCard;
	enum class HeaderOptions : unsigned {
		BasicInfo = 1 << 0,
		FileldInfo = 1 << 1,
		TypeInfo = 1 << 2,
		PathsDict = 1 << 3,
	};
	unsigned headerOptions = static_cast<unsigned>(HeaderOptions::BasicInfo);
	int maxRecordCount = 1000;
	bool withSnapshot = false;

	ShvJournalGetLogParams() {}
	ShvJournalGetLogParams(const shv::chainpack::RpcValue &opts);

	shv::chainpack::RpcValue toRpcValue() const;
};

} // namespace utils
} // namespace iotqt
} // namespace shv

#endif // SHV_IOTQT_UTILS_SHVJOURNALGETLOGPARAMS_H
