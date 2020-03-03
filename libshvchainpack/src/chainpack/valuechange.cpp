#include "valuechange.h"

namespace shv {
namespace chainpack {

//================================================================
// ValueChange
//================================================================
ValueChange::MetaType::MetaType()
	: Super("ValueChange")
{
	m_keys = {
		RPC_META_KEY_DEF(Value),
		RPC_META_KEY_DEF(DateTime),
		RPC_META_KEY_DEF(ShortTime),
	};
	//m_tags = {
	//	RPC_META_TAG_DEF(shvPath)
	//};
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
	: Super(RpcValue::IMap())
{
	//MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	setValue(val);
	setShortTime(short_time);
}

ValueChange::ValueChange(const RpcValue &val, const RpcValue::DateTime &date_time)
	: Super(RpcValue::IMap())
{
	//MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	setValue(val);
	setDateTime(date_time);
}

ValueChange::ValueChange(const RpcValue &val, const RpcValue::DateTime &date_time, unsigned short_time)
	: Super(RpcValue::IMap())
{
	//MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	setValue(val);
	setDateTime(date_time);
	setShortTime(short_time);
}

ValueChange::ValueChange(const RpcValue &o)
	: Super(o)
{
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
}

} // namespace chainpack
} // namespace shv
