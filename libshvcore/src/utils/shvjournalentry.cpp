#include "shvjournalentry.h"
#include "shvlogheader.h"
#include "shvlogtypeinfo.h"

namespace shv {
namespace core {
namespace utils {

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

chainpack::DataChange ShvJournalEntry::toDataChange() const
{
	shv::chainpack::DataChange ret(value, chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(epochMsec), shortTime);
	//ret.setDomain(domain);
	ret.setValueFlags(valueFlags);
	return ret;
}

} // namespace utils
} // namespace core
} // namespace shv
