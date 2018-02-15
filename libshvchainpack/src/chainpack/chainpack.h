#pragma once

#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPack
{
public:
	static constexpr uint8_t ARRAY_FLAG_MASK = 64;
	struct SHVCHAINPACK_DECL_EXPORT TypeInfo {
		enum Enum {
			INVALID = -1,

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

			FALSE = 253,
			TRUE = 254,
			TERM = 255,
		};
		static const char* name(Enum e);
	};

	static RpcValue::Type typeInfoToType(TypeInfo::Enum type_info);
};

}}

