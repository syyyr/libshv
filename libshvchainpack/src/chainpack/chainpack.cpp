#include "chainpack.h"

namespace shv {
namespace chainpack {

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
