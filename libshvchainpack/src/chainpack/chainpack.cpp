#include "chainpack.h"

namespace shv {
namespace chainpack {

RpcValue::Type ChainPack::typeInfoToType(ChainPack::TypeInfo::Enum type_info)
{
	switch (type_info) {
	case ChainPack::TypeInfo::Null: return RpcValue::Type::Null;
	case ChainPack::TypeInfo::UInt: return RpcValue::Type::UInt;
	case ChainPack::TypeInfo::Int: return RpcValue::Type::Int;
	case ChainPack::TypeInfo::Double: return RpcValue::Type::Double;
	case ChainPack::TypeInfo::Bool: return RpcValue::Type::Bool;
	case ChainPack::TypeInfo::Blob: return RpcValue::Type::Blob;
	case ChainPack::TypeInfo::String: return RpcValue::Type::String;
	case ChainPack::TypeInfo::DateTimeEpoch: return RpcValue::Type::DateTime; // deprecated
	case ChainPack::TypeInfo::DateTime: return RpcValue::Type::DateTime;
	case ChainPack::TypeInfo::List: return RpcValue::Type::List;
	case ChainPack::TypeInfo::Map: return RpcValue::Type::Map;
	case ChainPack::TypeInfo::IMap: return RpcValue::Type::IMap;
	//case ChainPackProtocol::TypeInfo::MetaIMap: return RpcValue::Type::MetaIMap;
	case ChainPack::TypeInfo::Decimal: return RpcValue::Type::Decimal;
	default:
		SHVCHP_EXCEPTION(std::string("There is type for type info ") + ChainPack::TypeInfo::name(type_info));
	}
}

const char *ChainPack::TypeInfo::name(ChainPack::TypeInfo::Enum e)
{
	switch (e) {
	case INVALID: return "INVALID";
	case FALSE: return "FALSE";
	case TRUE: return "TRUE";

	case Null: return "Null";
	case UInt: return "UInt";
	case Int: return "Int";
	case Double: return "Double";
	case Bool: return "Bool";
	case Blob: return "Blob";
	case String: return "String";
	case List: return "List";
	case Map: return "Map";
	case IMap: return "IMap";
	case DateTimeEpoch: return "DateTimeEpoch";
	case DateTime: return "DateTime";
	case MetaIMap: return "MetaIMap";
	case MetaSMap: return "MetaSMap";
	case Decimal: return "Decimal";

	case TERM: return "TERM";
	}
	//SHVCHP_EXCEPTION("Unknown TypeInfo: " + Utils::toString((int)e));
	return "";
}

}}
