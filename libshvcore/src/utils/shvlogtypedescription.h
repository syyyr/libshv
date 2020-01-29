#ifndef SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
#define SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H

#include "../shvcoreglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <string>

namespace shv {
namespace core {
namespace utils {

struct SHVCORE_DECL_EXPORT ShvLogTypeDescription
{
	enum class Type {
		Invalid,
		BitField,
		Enum,
		Bool,
		UInt,
		Int,
		Decimal,
		Double,
		String,
		DateTime,
		List,
		Map,
		IMap,
	};
	Type type = Type::Invalid;
	enum class SampleType {Continuous , Discrete};
	SampleType sampleType = SampleType::Continuous;
	std::string description;
	shv::chainpack::RpcValue::List fields;

	static const std::string typeToString(Type t);
	static Type typeFromString(const std::string &s);

	static const std::string sampleTypeToString(SampleType t);
	static SampleType sampleTypeFromString(const std::string &s);

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeDescription fromRpcValue(const chainpack::RpcValue &v);
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
