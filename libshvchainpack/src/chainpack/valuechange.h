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
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::ValueChange};
		struct Tag { enum Enum {DateTime = chainpack::meta::Tag::USER,
								ShortTime,
								MAX};};
		//struct Key { enum Enum {Value = 1, DateTime, ShortTime, MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	ValueChange() {}
	ValueChange(const RpcValue &val, unsigned short_time);
	ValueChange(const RpcValue &val, const shv::chainpack::RpcValue::DateTime &date_time);
	ValueChange(const RpcValue &val, const shv::chainpack::RpcValue::DateTime &date_time, unsigned short_time);
	ValueChange(const RpcValue &o);

	bool hasDateTime() const { return metaData().hasKey(MetaType::Tag::DateTime); }
	shv::chainpack::RpcValue::DateTime dateTime() const { return metaValue(MetaType::Tag::DateTime).toDateTime(); }
	void setDateTime(const shv::chainpack::RpcValue::DateTime &dt) { setMetaValue(MetaType::Tag::DateTime, dt); }

	bool hasShortTime() const { return metaData().hasKey(MetaType::Tag::ShortTime); }
	unsigned shortTime() const { return metaValue(MetaType::Tag::ShortTime).toUInt(); }
	void setShortTime(unsigned t) { setMetaValue(MetaType::Tag::ShortTime, t); }
};

} // namespace chainpack
} // namespace shv

