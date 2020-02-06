#include "shvjournalentry.h"
#include "shvlogtypeinfo.h"

namespace shv {
namespace core {
namespace utils {

//==============================================================
// ShvJournalEntry
//==============================================================
const char *ShvJournalEntry::DOMAIN_VAL_CHANGE = "C";
const char *ShvJournalEntry::DOMAIN_VAL_FASTCHANGE = "F";
const char *ShvJournalEntry::DOMAIN_VAL_SERVICECHANGE = "S";

chainpack::RpcValue ShvJournalEntry::toRpcValueMap() const
{
	chainpack::RpcValue::Map m;
	m["epochMsec"] = epochMsec;
	if(!path.empty())
		m["path"] = path;
	if(value.isValid())
		m["value"] = value;
	if(shortTime != NO_SHORT_TIME)
		m["shortTime"] = shortTime;
	if(!domain.empty())
		m["domain"] = domain;
	if(sampleType != ShvLogTypeDescr::SampleType::Continuous)
		m["sampleType"] = ShvLogTypeDescr::sampleTypeToString(sampleType);
	return std::move(m);
}

} // namespace utils
} // namespace core
} // namespace shv
