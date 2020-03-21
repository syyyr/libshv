#include "valuechange.h"

#include <necrolog.h>

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

ValueChange::ValueChange(const RpcValue &val, const RpcValue &date_time)
	: ValueChange(val, date_time, RpcValue())
{
}

ValueChange::ValueChange(const RpcValue &val, const RpcValue &date_time, const RpcValue &short_time)
	: m_value(val)
	, m_dateTime(date_time)
	, m_shortTime(short_time)
{
	if(isValueChange(val))
		nWarning() << "Storing ValueChange to value (ValueChange inside ValueChange):" << val.toCpon();
}

void ValueChange::setValue(const RpcValue &val)
{
	m_value = val;
	if(isValueChange(val))
		nWarning() << "Storing ValueChange to value (ValueChange inside ValueChange):" << val.toCpon();
}

ValueChange ValueChange::fromRpcValue(const RpcValue &val)
{
	if(isValueChange(val)) {
		ValueChange ret;
		if(val.isList()) {
			const RpcValue::List &lst = val.toList();
			if(lst.size() == 1) {
				RpcValue wrapped_val = val.toList().value(0);
				if(!wrapped_val.metaData().isEmpty()) {
					ret.setValue(wrapped_val.clone(RpcValue::CloneMetaData));
					goto set_meta_data;
				}
			}
		}
		ret.setValue(val.clone(!RpcValue::CloneMetaData));
set_meta_data:
		ret.setDateTime(val.metaValue(MetaType::Tag::DateTime));
		ret.setShortTime(val.metaValue(MetaType::Tag::ShortTime));
		return ret;
	}
	return ValueChange(val.clone(RpcValue::CloneMetaData), RpcValue());
}

RpcValue ValueChange::toRpcValue() const
{
	RpcValue ret;
	if(m_value.isValid()) {
		if(m_value.metaData().isEmpty())
			ret = m_value.clone();
		else
			ret = RpcValue::List{m_value};
	}
	else {
		ret = RpcValue{nullptr};
	}
	ret.setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	if(m_dateTime.isDateTime())
		ret.setMetaValue(MetaType::Tag::DateTime, m_dateTime.toDateTime());
	if(m_shortTime.isUInt())
		ret.setMetaValue(MetaType::Tag::ShortTime, m_shortTime);
	return ret;
}

} // namespace chainpack
} // namespace shv
