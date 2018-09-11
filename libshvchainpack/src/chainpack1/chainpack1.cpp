#include "chainpack1.h"

namespace shv {
namespace chainpack1 {

RpcValue::Type ChainPack::typeInfoToArrayType(ChainPack::PackingSchema::Enum type_info)
{
	switch (type_info) {
	case ChainPack::PackingSchema::Null: return RpcValue::Type::Null;
	case ChainPack::PackingSchema::UInt: return RpcValue::Type::UInt;
	case ChainPack::PackingSchema::Int: return RpcValue::Type::Int;
	case ChainPack::PackingSchema::Double: return RpcValue::Type::Double;
	case ChainPack::PackingSchema::Bool: return RpcValue::Type::Bool;
	//case ChainPack::TypeInfo::Blob: return RpcValue::Type::Blob;
	case ChainPack::PackingSchema::String: return RpcValue::Type::String;
	case ChainPack::PackingSchema::CString: return RpcValue::Type::String;
	//case ChainPack::TypeInfo::DateTimeEpoch: return RpcValue::Type::DateTime; // deprecated
	case ChainPack::PackingSchema::DateTime: return RpcValue::Type::DateTime;
	case ChainPack::PackingSchema::List: return RpcValue::Type::List;
	case ChainPack::PackingSchema::Map: return RpcValue::Type::Map;
	case ChainPack::PackingSchema::IMap: return RpcValue::Type::IMap;
	//case ChainPackProtocol::TypeInfo::MetaIMap: return RpcValue::Type::MetaIMap;
	case ChainPack::PackingSchema::Decimal: return RpcValue::Type::Decimal;
	default:
		SHVCHP_EXCEPTION(std::string("There is type for type info ") + ChainPack::PackingSchema::name(type_info));
	}
}

const char *ChainPack::PackingSchema::name(ChainPack::PackingSchema::Enum e)
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
	case Blob_depr: return "Blob_depr";
	case String: return "String";
	case CString: return "CString";
	case List: return "List";
	case Map: return "Map";
	case IMap: return "IMap";
	case DateTimeEpoch_depr: return "DateTimeEpoch_depr";
	case DateTime: return "DateTime";
	case MetaMap: return "MetaMap";
	//case MetaSMap: return "MetaSMap";
	case Decimal: return "Decimal";

	case TERM: return "TERM";
	}
	//SHVCHP_EXCEPTION("Unknown TypeInfo: " + Utils::toString((int)e));
	return "";
}

}}
