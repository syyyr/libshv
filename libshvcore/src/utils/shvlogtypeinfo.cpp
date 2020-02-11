#include "shvlogtypeinfo.h"

namespace shv {
namespace core {
namespace utils {

//=====================================================================
// ShvLogTypeDescrField
//=====================================================================
chainpack::RpcValue ShvLogTypeDescrField::toRpcValue() const
{
	chainpack::RpcValue::Map m;
	m["name"] = name;
	if(!typeName.empty())
		m["typeName"] = typeName;
	if(!description.empty())
		m["description"] = description;
	if(value.isValid())
		m["value"] = value;
	return std::move(m);
}

ShvLogTypeDescrField ShvLogTypeDescrField::fromRpcValue(const chainpack::RpcValue &v)
{
	ShvLogTypeDescrField ret;
	if(v.isMap()) {
		const chainpack::RpcValue::Map &m = v.toMap();
		ret.name = m.value("name").toString();
		ret.description = m.value("description").toString();
		ret.typeName = m.value("typeName").toString();
		ret.value = m.value("value");
	}
	return ret;
}

//=====================================================================
// ShvLogTypeDescr
//=====================================================================
const std::string ShvLogTypeDescr::typeToString(ShvLogTypeDescr::Type t)
{
	switch (t) {
	case Type::Invalid: break;
	case Type::BitField: return "BitField";
	case Type::Enum: return "Enum";
	case Type::Bool: return "Bool";
	case Type::UInt: return "UInt";
	case Type::Int: return "Int";
	case Type::Decimal: return "Decimal";
	case Type::Double: return "Double";
	case Type::String: return "String";
	case Type::DateTime: return "DateTime";
	case Type::List: return "List";
	case Type::Map: return "Map";
	case Type::IMap: return "IMap";
	}
	return "Invalid";
}

ShvLogTypeDescr::Type ShvLogTypeDescr::typeFromString(const std::string &s)
{
	if(s == "BitField") return Type::BitField;
	if(s == "Enum") return Type::Enum;
	if(s == "Bool") return Type::Bool;
	if(s == "UInt") return Type::UInt;
	if(s == "Int") return Type::Int;
	if(s == "Decimal") return Type::Decimal;
	if(s == "Double") return Type::Double;
	if(s == "String") return Type::String;
	if(s == "DateTime") return Type::DateTime;
	if(s == "List") return Type::List;
	if(s == "Map") return Type::Map;
	if(s == "IMap") return Type::IMap;
	return Type::Invalid;
}

const std::string ShvLogTypeDescr::sampleTypeToString(ShvLogTypeDescr::SampleType t)
{
	switch (t) {
	case SampleType::Discrete: break;
	case SampleType::Continuous: return "Continuous";
	case SampleType::Invalid: return "Invalid";
	}
	return "Discrete";
}

ShvLogTypeDescr::SampleType ShvLogTypeDescr::sampleTypeFromString(const std::string &s)
{
	if(s == "Invalid") return SampleType::Invalid;
	if(s == "Discrete") return SampleType::Discrete;
	return SampleType::Continuous;
}

chainpack::RpcValue ShvLogTypeDescr::toRpcValue() const
{
	chainpack::RpcValue::Map m;
	if(type != Type::Invalid)
		m["type"] = typeToString(type);
	if(sampleType != SampleType::Continuous)
		m["sampleType"] = sampleTypeToString(sampleType);
	if(!description.empty())
		m["description"] = description;
	if(!fields.empty()) {
		chainpack::RpcValue::List lst;
		for(const ShvLogTypeDescrField &fld : fields)
			lst.push_back(fld.toRpcValue());
		m["fields"] = std::move(lst);
	}
	if(minVal.isValid())
		m["minVal"] = minVal;
	if(maxVal.isValid())
		m["maxVal"] = maxVal;
	return std::move(m);
}

ShvLogTypeDescr ShvLogTypeDescr::fromRpcValue(const chainpack::RpcValue &v)
{
	ShvLogTypeDescr ret;
	if(v.isMap()) {
		const chainpack::RpcValue::Map &m = v.toMap();
		ret.type = typeFromString(m.value("type").toString());
		ret.sampleType = sampleTypeFromString(m.value("sampleType").toString());
		ret.description = m.value("description").toString();
		for(const auto &rv : m.value("fields").toList())
			ret.fields.push_back(ShvLogTypeDescrField::fromRpcValue(rv));
		ret.minVal = m.value("minVal");
		ret.maxVal = m.value("maxVal");
	}
	return ret;
}

//=====================================================================
// ShvLogPathDescr
//=====================================================================
chainpack::RpcValue ShvLogPathDescr::toRpcValue() const
{
	chainpack::RpcValue::Map m;
	m["type"] = typeName;
	if(!description.empty())
		m["description"] = description;
	return std::move(m);
}

ShvLogPathDescr ShvLogPathDescr::fromRpcValue(const chainpack::RpcValue &v)
{
	ShvLogPathDescr ret;
	if(v.isMap()) {
		const chainpack::RpcValue::Map &m = v.toMap();
		ret.typeName = m.value("type").toString();
		ret.description = m.value("description").toString();
	}
	return ret;
}

//=====================================================================
// ShvLogTypeInfo
//=====================================================================
chainpack::RpcValue ShvLogTypeInfo::toRpcValue() const
{
	chainpack::RpcValue::Map m;
	chainpack::RpcValue::Map mt;
	for(const auto &kv : types) {
		mt[kv.first] = kv.second.toRpcValue();
	}
	m["types"] = std::move(mt);
	chainpack::RpcValue::Map mp;
	for(const auto &kv : paths) {
		mp[kv.first] = kv.second.toRpcValue();
	}
	m["paths"] = std::move(mp);
	return std::move(m);
}

ShvLogTypeInfo ShvLogTypeInfo::fromRpcValue(const chainpack::RpcValue &v)
{
	ShvLogTypeInfo ret;
	const chainpack::RpcValue::Map &m = v.toMap();
	const chainpack::RpcValue::Map &mt = m.value("types").toMap();
	for(const auto &kv : mt) {
		ret.types[kv.first] = ShvLogTypeDescr::fromRpcValue(kv.second);
	}
	const chainpack::RpcValue::Map &mp = m.value("paths").toMap();
	for(const auto &kv : mp) {
		ret.paths[kv.first] = ShvLogPathDescr::fromRpcValue(kv.second);
	}
	return ret;
}

} // namespace utils
} // namespace core
} // namespace shv
