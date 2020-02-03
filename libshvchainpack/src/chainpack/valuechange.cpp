#include "valuechange.h"

namespace shv {
namespace chainpack {

//================================================================
// ValueChange
//================================================================
ValueChange::MetaType::MetaType()
	: Super("ValueChange")
{
	/*
	m_keys = {
		RPC_META_KEY_DEF(Value),
		RPC_META_KEY_DEF(DateTime),
		RPC_META_KEY_DEF(ShortTime),
	};
	*/
	m_tags = {
		RPC_META_TAG_DEF(DateTime),
		RPC_META_TAG_DEF(ShortTime)
	};
}

void ValueChange::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

ValueChange::ValueChange(const RpcValue &val, unsigned short_time)
	: Super(val)
{
	MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	setMetaValue(MetaType::Tag::ShortTime, short_time);
}

ValueChange::ValueChange(const RpcValue &val, const RpcValue::DateTime &date_time)
	: Super(val)
{
	MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	setMetaValue(MetaType::Tag::DateTime, std::move(date_time));
}

ValueChange::ValueChange(const RpcValue &val, const RpcValue::DateTime &date_time, unsigned short_time)
	: Super(val)
{
	MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	setMetaValue(MetaType::Tag::ShortTime, short_time);
	setMetaValue(MetaType::Tag::DateTime, std::move(date_time));
}

ValueChange::ValueChange(const RpcValue &o)
	: Super(o)
{
}

} // namespace chainpack
} // namespace shv
