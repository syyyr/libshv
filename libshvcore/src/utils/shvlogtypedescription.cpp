#include "shvlogtypedescription.h"

namespace shv {
namespace core {
namespace utils {

const std::string ShvLogTypeDescription::typeToString(ShvLogTypeDescription::Type t)
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

ShvLogTypeDescription::Type ShvLogTypeDescription::typeFromString(const std::string &s)
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

const std::string ShvLogTypeDescription::sampleTypeToString(ShvLogTypeDescription::SampleType t)
{
	switch (t) {
	case SampleType::Discrete: break;
	case SampleType::Continuous: return "Continuous";
	}
	return "Discrete";
}

ShvLogTypeDescription::SampleType ShvLogTypeDescription::sampleTypeFromString(const std::string &s)
{
	if(s == "Discrete") return SampleType::Discrete;
	return SampleType::Continuous;
}

chainpack::RpcValue ShvLogTypeDescription::toRpcValue() const
{
	chainpack::RpcValue::Map m;
	if(type != Type::Invalid)
		m["type"] = typeToString(type);
	if(sampleType != SampleType::Continuous)
		m["sampleType"] = sampleTypeToString(sampleType);
	if(!description.empty())
		m["description"] = description;
	if(!fields.empty())
		m["fields"] = fields;
	return std::move(m);
}

ShvLogTypeDescription ShvLogTypeDescription::fromRpcValue(const chainpack::RpcValue &v)
{
	ShvLogTypeDescription ret;
	if(v.isMap()) {
		const chainpack::RpcValue::Map &m = v.toMap();
		ret.type = typeFromString(m.value("type").toString());
		ret.sampleType = sampleTypeFromString(m.value("sampleType").toString());
		ret.description = m.value("description").toString();
		ret.fields = m.value("fields").toList();
	}
	return ret;
}

} // namespace utils
} // namespace core
} // namespace shv
