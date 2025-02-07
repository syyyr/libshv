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
		//RPC_META_TAG_DEF(Domain),
		RPC_META_TAG_DEF(ValueFlags),
		RPC_META_TAG_DEF(SpecialListValue),
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
const char *DataChange::valueFlagToString(DataChange::ValueFlag flag)
{
	switch (flag) {
	case ValueFlag::Snapshot: return "Snapshot";
	case ValueFlag::Spontaneous: return "Spontaneous";
	case ValueFlag::ValueFlagCount: return "ValueFlagCount";
	}
	return "???";
}

std::string DataChange::valueFlagsToString(ValueFlags st)
{
	std::string ret;
	auto append_str = [&ret](const char *str) {
		if(!ret.empty())
			ret += ',';
		ret += str;
	};
	for (int flag = 0; flag < ValueFlagCount; ++flag) {
		unsigned mask = 1 << flag;
		if(st & mask)
			append_str(valueFlagToString((ValueFlag)flag));
	}
	return ret;
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

bool DataChange::isDataChange(const RpcValue &rv)
{
	return rv.metaTypeId() == MetaType::ID
			&& rv.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID;
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
			const RpcValue::List &lst = val.asList();
			if(lst.size() == 1) {
				RpcValue wrapped_val = lst.value(0);
				if(!wrapped_val.metaData().isEmpty() && !val.metaValue(MetaType::Tag::SpecialListValue).isValid()) {
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
		//ret.setDomain(val.metaValue(MetaType::Tag::Domain).asString());
		int st = val.metaValue(MetaType::Tag::ValueFlags).toInt();
		ret.setValueFlags(st);
		return ret;
	}
	return DataChange(val, RpcValue::DateTime());
}

RpcValue DataChange::toRpcValue() const
{
	RpcValue ret;
	if(m_value.isValid()) {
		if(m_value.metaData().isEmpty()) {
			ret = m_value;
			const RpcValue::List &lst = m_value.asList();
			if(lst.size() == 1 && !lst[0].metaData().isEmpty()) {
				// [<meta>value] will be packed as <dc-meta>[<meta>value]
				// what will be unpacked to DataChange as <meta>value
				// mark such a rare case with tag SpecialListValue
				// to avoid this mis-interpretation
				ret.setMetaValue(MetaType::Tag::SpecialListValue, true);
			}
		}
		else {
			ret = RpcValue::List{m_value};
		}
	}
	else {
		ret = RpcValue{nullptr};
	}
	ret.setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	if(hasDateTime())
		ret.setMetaValue(MetaType::Tag::DateTime, m_dateTime);
	if(hasShortTime())
		ret.setMetaValue(MetaType::Tag::ShortTime, (unsigned)m_shortTime);
	//if(hasDomain())
	//	ret.setMetaValue(MetaType::Tag::Domain, m_domain);
	if(valueFlags() != 0)
		ret.setMetaValue(MetaType::Tag::ValueFlags, (int)valueFlags());
	return ret;
}

bool DataChange::testBit(const unsigned &n, int pos)
{
	unsigned mask = 1 << pos;
	return n & mask;
}

void DataChange::setBit(unsigned &n, int pos, bool b)
{
	unsigned mask = 1 << pos;
	n &= ~mask;
	if(b)
		n |= mask;
}

} // namespace chainpack
} // namespace shv
