#pragma once

#include "abstractstreamreader.h"
#include "abstractstreamwriter.h"
#include "rpcvalue.h"

//#include <string>
//#include <streambuf>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPack
{
public: // public just because of testing, normal user do not need to use this enum
	struct SHVCHAINPACK_DECL_EXPORT TypeInfo {
		enum Enum {
			INVALID = -1,
			/// types
			Null = 128,
			UInt,
			Int,
			Double,
			Bool,
			Blob,
			String,
			DateTimeEpoch, // deprecated
			List,
			Map,
			IMap,
			MetaIMap,
			Decimal,
			DateTime,
			MetaSMap,
			/// arrays
			/*
			Null_Array = Null | ARRAY_FLAG_MASK, // if bit 6 is set, then packed value is an Array of corresponding values
			UInt_Array = UInt | ARRAY_FLAG_MASK,
			Int_Array = Int | ARRAY_FLAG_MASK,
			Double_Array = Double | ARRAY_FLAG_MASK,
			Bool_Array = Bool | ARRAY_FLAG_MASK,
			Blob_Array = Blob | ARRAY_FLAG_MASK,
			String_Array = String | ARRAY_FLAG_MASK,
			DateTimeEpoch_Array = DateTimeEpoch | ARRAY_FLAG_MASK, // not used
			List_Array = List | ARRAY_FLAG_MASK,
			Map_Array = Map | ARRAY_FLAG_MASK,
			IMap_Array = IMap | ARRAY_FLAG_MASK,
			MetaIMap_Array = MetaIMap | ARRAY_FLAG_MASK,
			Decimal_Array = Decimal | ARRAY_FLAG_MASK,
			DateTimeUtc_Array = DateTimeUtc | ARRAY_FLAG_MASK, // not used, DateTimeTZ_Array is used allways for DateTime arrays
			DateTimeTZ_Array = DateTimeTZ | ARRAY_FLAG_MASK,
			*/
			FALSE = 253,
			TRUE = 254,
			TERM = 255,
		};
		static const char* name(Enum e);
	};

	static constexpr uint8_t ARRAY_FLAG_MASK = 64;
	static TypeInfo::Enum typeToTypeInfo(RpcValue::Type tid);
	static RpcValue::Type typeInfoToType(TypeInfo::Enum type_info);
};

}}

