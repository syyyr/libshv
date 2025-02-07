#include "shvjournalentry.h"
#include "shvlogheader.h"
#include "shvlogtypeinfo.h"

namespace shv {
namespace core {
namespace utils {

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
const char *ShvJournalEntry::DOMAIN_VAL_CHANGE = shv::chainpack::Rpc::SIG_VAL_CHANGED;
const char *ShvJournalEntry::DOMAIN_VAL_FASTCHANGE = shv::chainpack::Rpc::SIG_VAL_FASTCHANGED;
const char *ShvJournalEntry::DOMAIN_VAL_SERVICECHANGE = shv::chainpack::Rpc::SIG_SERVICE_VAL_CHANGED;
const char *ShvJournalEntry::DOMAIN_SHV_SYSTEM = "SHV_SYS";
const char *ShvJournalEntry::DOMAIN_SHV_COMMAND = shv::chainpack::Rpc::SIG_COMMAND_LOGGED;

const char* ShvJournalEntry::PATH_APP_START = "APP_START";
//const char* ShvJournalEntry::PATH_SNAPSHOT_BEGIN = "SNAPSHOT_BEGIN";
//const char* ShvJournalEntry::PATH_SNAPSHOT_END = "SNAPSHOT_END";
const char* ShvJournalEntry::PATH_DATA_MISSING = "DATA_MISSING";
const char* ShvJournalEntry::PATH_DATA_DIRTY = "DATA_DIRTY";

const char* ShvJournalEntry::DATA_MISSING_UNAVAILABLE = "Unavailable";
const char* ShvJournalEntry::DATA_MISSING_NOT_EXISTS = "NotExists";

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

chainpack::DataChange ShvJournalEntry::toDataChange() const
{
	if(domain == shv::chainpack::Rpc::SIG_VAL_CHANGED) {
		shv::chainpack::DataChange ret(value, shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(epochMsec), shortTime);
		ret.setValueFlags(valueFlags);
		return ret;
	}
	return {};
}

} // namespace utils
} // namespace core
} // namespace shv
