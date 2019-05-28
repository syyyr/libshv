#pragma once

#include "../shvchainpackglobal.h"

#include "rpcvalue.h"
#include "utils.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ValueChange : public shv::chainpack::RpcValue
{
	using Super = shv::chainpack::RpcValue;
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::RegisteredMetaTypes::ValueChange};
		/*
		struct Tag { enum Enum {RequestId = chainpack::meta::Tag::USER, // 8
								MAX};};
		*/
		struct Key { enum Enum {Value = 1, DateTime, ShortTime, MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::Value, v, setV, alue)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue::DateTime, MetaType::Key::DateTime, d, setD, ateTime)
	SHV_IMAP_FIELD_IMPL(unsigned, MetaType::Key::ShortTime, s, setS, hortTime)
public:
	//using Super::Super;
	ValueChange() {}
	ValueChange(const RpcValue &val, unsigned short_time);
	ValueChange(const RpcValue &val, const shv::chainpack::RpcValue::DateTime &date_time);
	ValueChange(const RpcValue &val, const shv::chainpack::RpcValue::DateTime &date_time, unsigned short_time);
	ValueChange(const RpcValue &o);
};

} // namespace chainpack
} // namespace shv

