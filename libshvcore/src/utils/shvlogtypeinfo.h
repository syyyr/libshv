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
	chainpack::RpcValue::Map tags;

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
	enum class SampleType {Invalid = 0, Continuous , Discrete};
	SampleType sampleType = SampleType::Continuous;

	static const char* OPT_MIN_VAL;
	static const char* OPT_MAX_VAL;
	static const char* OPT_DEC_PLACES;
	chainpack::RpcValue::Map tags;

	ShvLogTypeDescr() {}
	ShvLogTypeDescr(const std::string &type_name) : type(typeFromString(type_name)) { }
	ShvLogTypeDescr(Type t, std::vector<ShvLogTypeDescrField> &&flds, SampleType st = SampleType::Continuous, const chainpack::RpcValue::Map &tags = chainpack::RpcValue::Map(), const std::string &descr = std::string())
	    : ShvLogTypeDescr(t, std::move(flds), chainpack::RpcValue::Map(tags), descr, st) {}
	ShvLogTypeDescr(Type t, SampleType st = SampleType::Continuous, const chainpack::RpcValue::Map &tags = chainpack::RpcValue::Map(), const std::string &descr = std::string())
	    : ShvLogTypeDescr(t, std::vector<ShvLogTypeDescrField>(), chainpack::RpcValue::Map(tags), descr, st) {}
	ShvLogTypeDescr(Type t, std::vector<ShvLogTypeDescrField> &&flds, chainpack::RpcValue::Map &&tags, const std::string &descr = std::string(), SampleType st = SampleType::Continuous)
		: fields(std::move(flds))
		, description(descr)
		, type(t)
		, sampleType(st)
		, tags(std::move(tags))
	{
		//tags.setValue(OPT_MIN_VAL, min_val);
		//tags.setValue(OPT_MAX_VAL, max_val);
	}

	bool isValid() const { return type != Type::Invalid; }

	static const std::string typeToString(Type t);
	static Type typeFromString(const std::string &s);

	static const std::string sampleTypeToString(SampleType t);
	static SampleType sampleTypeFromString(const std::string &s);

	void applyTags(const chainpack::RpcValue::Map &t);
	chainpack::RpcValue defaultRpcValue() const;

	std::string unit() const;
	std::string visualStyleName() const;
	std::string alarm() const;
	int decimalPlaces() const;

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeDescr fromRpcValue(const chainpack::RpcValue &v);
};

struct SHVCORE_DECL_EXPORT ShvLogPathDescr
{
	std::string typeName;
	std::string description;
	std::string systemPath;
	chainpack::RpcValue::Map tags;

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
	std::vector<std::string> systemPaths;

	ShvLogTypeInfo() {}
	ShvLogTypeInfo(std::map<std::string, ShvLogTypeDescr> &&types, std::map<std::string, ShvLogPathDescr> &&paths)
		: types(std::move(types))
		, paths(std::move(paths))
	{}

	bool isEmpty() const { return types.size() == 0 && paths.size() == 0; }

	ShvLogTypeDescr typeDescription(const std::string &shv_path) const;
	std::string findSystemPath(const std::string &shv_path) const;

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeInfo fromRpcValue(const chainpack::RpcValue &v);
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
