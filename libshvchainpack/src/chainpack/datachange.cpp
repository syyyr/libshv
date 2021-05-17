#include "datachange.h"

#include <necrolog.h>

namespace shv {
namespace chainpack {

//================================================================
// ValueChange
//================================================================
DataChange::MetaType::MetaType()
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
		RPC_META_TAG_DEF(ShortTime),
		RPC_META_TAG_DEF(Domain),
		RPC_META_TAG_DEF(SampleType),
	};
}

void DataChange::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}
/*
DataChange::DataChange(const RpcValue &val)
{
	setValue(val);
}
*/
const char *DataChange::sampleTypeToString(DataChange::SampleType st)
{
	switch (st) {
	case SampleType::Invalid: return "";
	case SampleType::Continuous: return "Continuous";
	case SampleType::Discrete: return "Discrete";
	}
	return "???";
}

DataChange::DataChange(const RpcValue &val, const RpcValue::DateTime &date_time, int short_time)
	: m_dateTime(date_time)
	, m_shortTime(short_time)
{
	setValue(val);
}

DataChange::DataChange(const RpcValue &val, unsigned short_time)
	: m_shortTime((int)short_time)
{
	setValue(val);
}

void DataChange::setValue(const RpcValue &val)
{
	m_value = val;
	if(isDataChange(val))
		nWarning() << "Storing ValueChange to value (ValueChange inside ValueChange):" << val.toCpon();
}

DataChange DataChange::fromRpcValue(const RpcValue &val)
{
	if(isDataChange(val)) {
		DataChange ret;
		if(val.isList()) {
			const RpcValue::List &lst = val.toList();
			if(lst.size() == 1) {
				// we cannot make DataChange from RpcValue with meta-data
				// in this case, the inner DataChange must be wrapped in the list
				// val is in form <data-change>[<meta-data>value]
				RpcValue wrapped_val = lst.value(0);
				if(!wrapped_val.metaData().isEmpty()) {
					ret.setValue(wrapped_val);
					goto set_meta_data;
				}
			}
		}
		{
			//nInfo() << val.toCpon();
			RpcValue raw_val = val.metaStripped();
			ret.setValue(raw_val);
		}
set_meta_data:
		ret.setDateTime(val.metaValue(MetaType::Tag::DateTime));
		ret.setShortTime(val.metaValue(MetaType::Tag::ShortTime));
		ret.setDomain(val.metaValue(MetaType::Tag::Domain).asString());
		int st = val.metaValue(MetaType::Tag::SampleType).toInt();
		ret.setSampleType(st == (int)SampleType::Discrete? SampleType::Discrete: SampleType::Continuous);
		return ret;
	}
	return DataChange(val, RpcValue::DateTime());
}

RpcValue DataChange::toRpcValue() const
{
	RpcValue ret;
	if(m_value.isValid()) {
		if(m_value.metaData().isEmpty())
			ret = m_value;
		else
			ret = RpcValue::List{m_value};
	}
	else {
		ret = RpcValue{nullptr};
	}
	ret.setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	if(hasDateTime())
		ret.setMetaValue(MetaType::Tag::DateTime, m_dateTime);
	if(hasShortTime())
		ret.setMetaValue(MetaType::Tag::ShortTime, (unsigned)m_shortTime);
	if(hasDomain())
		ret.setMetaValue(MetaType::Tag::Domain, m_domain);
	if(sampleType() == SampleType::Discrete)
		ret.setMetaValue(MetaType::Tag::SampleType, (int)m_sampleType);
	return ret;
}

} // namespace chainpack
} // namespace shv
