#ifndef SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
#define SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H

#include "../shvcoreglobal.h"

#include <shv/chainpack/datachange.h>
//#include <shv/chainpack/rpcvalue.h>

#include <string>

namespace shv {
namespace core {
namespace utils {

struct SHVCORE_DECL_EXPORT ShvLogTypeDescrField
{
	std::string name;
	std::string typeName;
	std::string description;
	chainpack::RpcValue value;

	ShvLogTypeDescrField() {}
	//ShvLogTypeDescrField(const std::string &n)
	//	: ShvLogTypeDescrField(n, std::string(), chainpack::RpcValue(), std::string()) {}
	ShvLogTypeDescrField(const std::string &n, int bit_pos, const std::string &d = std::string())
		: ShvLogTypeDescrField(n, std::string(), bit_pos, d) {}
	//ShvLogTypeDescrField(const std::string &n, const std::string &type_name)
	//	: ShvLogTypeDescrField(n, type_name, chainpack::RpcValue(), std::string()) {}
	ShvLogTypeDescrField(const std::string &n, const std::string &type_name = std::string(), const chainpack::RpcValue &v = chainpack::RpcValue(), const std::string &descr = std::string())
		: name(n)
		, typeName(type_name)
		, description(descr)
		, value(v)
	{}

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeDescrField fromRpcValue(const chainpack::RpcValue &v);
};

struct SHVCORE_DECL_EXPORT ShvLogTypeDescr
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
	std::vector<ShvLogTypeDescrField> fields;
	std::string description;
	Type type = Type::Invalid;
	using SampleType = shv::chainpack::DataChange::SampleType;
	SampleType sampleType = SampleType::Continuous;
	chainpack::RpcValue minVal;
	chainpack::RpcValue maxVal;

	ShvLogTypeDescr() {}
	ShvLogTypeDescr(Type t, std::vector<ShvLogTypeDescrField> &&flds, const std::string &descr = std::string(), SampleType st = SampleType::Continuous)
		: ShvLogTypeDescr(t, std::move(flds), descr, st, chainpack::RpcValue(), chainpack::RpcValue()) {}
	ShvLogTypeDescr(Type t, std::vector<ShvLogTypeDescrField> &&flds, const std::string &descr, SampleType st, const chainpack::RpcValue &min_val, const chainpack::RpcValue &max_val)
		: fields(std::move(flds))
		, description(descr)
		, type(t)
		, sampleType(st)
		, minVal(min_val)
		, maxVal(max_val)
	{}

	static const std::string typeToString(Type t);
	static Type typeFromString(const std::string &s);

	static const std::string sampleTypeToString(SampleType t);
	static SampleType sampleTypeFromString(const std::string &s);

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeDescr fromRpcValue(const chainpack::RpcValue &v);
};

struct SHVCORE_DECL_EXPORT ShvLogPathDescr
{
	std::string typeName;
	std::string description;

	ShvLogPathDescr() {}
	ShvLogPathDescr(const std::string &t, const std::string &d = std::string())
		: typeName(t)
		, description(d)
	{}

	chainpack::RpcValue toRpcValue() const;
	static ShvLogPathDescr fromRpcValue(const chainpack::RpcValue &v);
};

struct SHVCORE_DECL_EXPORT ShvLogTypeInfo
{
	std::map<std::string, ShvLogTypeDescr> types; // type_name -> type_description
	std::map<std::string, ShvLogPathDescr> paths; // path -> type_name

	ShvLogTypeInfo() {}
	ShvLogTypeInfo(std::map<std::string, ShvLogTypeDescr> &&types, std::map<std::string, ShvLogPathDescr> &&paths)
		: types(std::move(types))
		, paths(std::move(paths))
	{}

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeInfo fromRpcValue(const chainpack::RpcValue &v);
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
