#include "shvjournalentry.h"
#include "shvlogheader.h"

namespace shv::core::utils {

ShvJournalEntry::MetaType::MetaType()
	: Super("ShvJournalEntry")
{
}

void ShvJournalEntry::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

//==============================================================
// ShvJournalEntry
//==============================================================
chainpack::RpcValue ShvJournalEntry::toRpcValueMap() const
{
	chainpack::RpcValue::Map m;
	m["epochMsec"] = epochMsec;
	if(epochMsec > 0)
		m[ShvLogHeader::Column::name(ShvLogHeader::Column::Timestamp)] = chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(epochMsec);
	if(!path.empty())
		m[ShvLogHeader::Column::name(ShvLogHeader::Column::Path)] = path;
	if(value.isValid())
		m[ShvLogHeader::Column::name(ShvLogHeader::Column::Value)] = value;
	if(shortTime != NO_SHORT_TIME)
		m[ShvLogHeader::Column::name(ShvLogHeader::Column::ShortTime)] = shortTime;
	if(!domain.empty())
		m[ShvLogHeader::Column::name(ShvLogHeader::Column::Domain)] = domain;
	if(valueFlags != 0)
		m[ShvLogHeader::Column::name(ShvLogHeader::Column::ValueFlags)] = valueFlags;
	if(!userId.empty())
		m[ShvLogHeader::Column::name(ShvLogHeader::Column::UserId)] = userId;
	return m;
}

chainpack::RpcValue ShvJournalEntry::toRpcValueList(std::function< chainpack::RpcValue (const std::string &)> map_path) const
{
	using namespace shv::chainpack;
	RpcValue::List rec;
	rec.push_back(RpcValue::DateTime::fromMSecsSinceEpoch(epochMsec));
	if(map_path)
		rec.push_back(map_path(path));
	else
		rec.push_back(path);
	rec.push_back(value);
	rec.push_back(shortTime == ShvJournalEntry::NO_SHORT_TIME ? RpcValue(nullptr): RpcValue(shortTime));
	rec.push_back((domain.empty() || domain == ShvJournalEntry::DOMAIN_VAL_CHANGE) ? RpcValue(nullptr): domain);
	rec.push_back(valueFlags);
	rec.push_back(userId.empty()? RpcValue(nullptr): RpcValue(userId));
	return rec;
}

bool ShvJournalEntry::isShvJournalEntry(const chainpack::RpcValue &rv)
{
	return rv.metaTypeId() == MetaType::ID
			&& rv.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID;
}

chainpack::RpcValue ShvJournalEntry::toRpcValue() const
{
	chainpack::RpcValue ret = toRpcValueMap();
	ret.setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	return ret;
}

ShvJournalEntry ShvJournalEntry::fromRpcValue(const shv::chainpack::RpcValue &rv)
{
	if(isShvJournalEntry(rv))
		return fromRpcValueMap(rv.asMap());
	return {};
}

ShvJournalEntry ShvJournalEntry::fromRpcValueMap(const chainpack::RpcValue::Map &m)
{
	ShvJournalEntry ret;
	// check timestamp first, it can contain time-zone, which is not supported currently, but we plan to do it in future
	chainpack::RpcValue::DateTime dt = m.value(ShvLogHeader::Column::name(ShvLogHeader::Column::Timestamp)).toDateTime();
	if(!dt.isZero())
		ret.epochMsec = dt.msecsSinceEpoch();
	else
		ret.epochMsec = m.value("epochMsec").toInt64();
	ret.path = m.value(ShvLogHeader::Column::name(ShvLogHeader::Column::Path)).asString();
	ret.value = m.value(ShvLogHeader::Column::name(ShvLogHeader::Column::Value));
	ret.shortTime = m.value(ShvLogHeader::Column::name(ShvLogHeader::Column::ShortTime), NO_SHORT_TIME).toInt();
	ret.domain = m.value(ShvLogHeader::Column::name(ShvLogHeader::Column::Domain)).asString();
	ret.valueFlags = m.value(ShvLogHeader::Column::name(ShvLogHeader::Column::ValueFlags)).toUInt();
	ret.userId = m.value(ShvLogHeader::Column::name(ShvLogHeader::Column::UserId)).asString();
	return ret;
}

ShvJournalEntry ShvJournalEntry::fromRpcValueList(const chainpack::RpcValue::List &row, std::function<std::string (const chainpack::RpcValue &)> unmap_path, std::string *err)
{
	using Column = ShvLogHeader::Column;
	chainpack::RpcValue dt = row.value(Column::Timestamp);
	if(!dt.isDateTime()) {
		if(err)
			*err = "Invalid date time, row: " + dt.toCpon();
		return {};
	}
	int64_t time = dt.toDateTime().msecsSinceEpoch();
	chainpack::RpcValue p = row.value(Column::Path);
	std::string path = unmap_path? unmap_path(p): p.asString();
	if(path.empty()) {
		if(err)
			*err = "Path dictionary corrupted, row: " + p.toCpon();
		return {};
	}
	//logDShvJournal() << "row:" << val.toCpon();
	ShvJournalEntry entry;
	entry.epochMsec = time;
	entry.path = path;
	entry.value = row.value(Column::Value);
	chainpack::RpcValue st = row.value(Column::ShortTime);
	entry.shortTime = (st.isInt() && st.toInt() >= 0)? st.toInt(): ShvJournalEntry::NO_SHORT_TIME;
	entry.domain = row.value(Column::Domain).asString();
	if(entry.domain.empty() || entry.domain == "C")
		entry.domain = ShvJournalEntry::DOMAIN_VAL_CHANGE;
	entry.valueFlags = row.value(Column::ValueFlags).toUInt();
	entry.userId = row.value(Column::UserId).asString();
	return entry;
}

chainpack::DataChange ShvJournalEntry::toDataChange() const
{
	if(domain == shv::chainpack::Rpc::SIG_VAL_CHANGED) {
		shv::chainpack::DataChange ret(value, shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(epochMsec), shortTime);
		ret.setValueFlags(valueFlags);
		return ret;
	}
	return {};
}

} // namespace shv
