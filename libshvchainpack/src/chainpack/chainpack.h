#pragma once

#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPack
{
public:
	// UTC msec since 2.2. 2018 folowed by signed UTC offset in 1/4 hour
	// Fri Feb 02 2018 00:00:00 == 1517529600 EPOCH
	static constexpr int64_t SHV_EPOCH_MSEC = 1517529600000;
public:
	static constexpr uint8_t ARRAY_FLAG_MASK = 64;
	static constexpr uint8_t STRING_META_KEY_PREFIX = 0xFE;
	struct SHVCHAINPACK_DECL_EXPORT PackingSchema {
		enum Enum {
			INVALID = -1,

			Null = 128,
			UInt,
			Int,
			Double,
			Bool,
			Blob_depr, // deprecated
			String,
			DateTimeEpoch_depr, // deprecated
			List,
			Map,
			IMap,
			MetaMap,
			Decimal,
			DateTime,
			CString,

			FALSE = 253,
			TRUE = 254,
			TERM = 255,
		};
		static const char* name(Enum e);
	};
};

}}

