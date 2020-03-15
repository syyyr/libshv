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
const char *ShvJournalEntry::DOMAIN_SHV_SYSTEM = "SHV_SYS";

const char* ShvJournalEntry::PATH_APP_START = "APP_START";
//const char* ShvJournalEntry::PATH_SNAPSHOT_END = "SNAPSHOT_END";
const char* ShvJournalEntry::PATH_DATA_MISSING = "DATA_MISSING";
const char* ShvJournalEntry::PATH_DATA_DIRTY = "DATA_DIRTY";

const char* ShvJournalEntry::DATA_MISSING_DEVICE_DISCONNECTED = "DeviceDisconnected_recoverable";
const char* ShvJournalEntry::DATA_MISSING_HP_DISCONNECTED = "HistoryProviderDisconnected_recoverable";
const char* ShvJournalEntry::DATA_MISSING_LOG_FILE_CORRUPTED = "LogFileCorrupted";
const char* ShvJournalEntry::DATA_MISSING_LOG_CACHE_FILE_MISSING = "LogCacheFileMissing_recoverable";
const char* ShvJournalEntry::DATA_MISSING_LOG_FILE_MISSING = "LogFileMissing";
const char* ShvJournalEntry::DATA_MISSING_APP_SHUTDOWN = "AppShutDown";

chainpack::RpcValue ShvJournalEntry::toRpcValueMap() const
{
	chainpack::RpcValue::Map m;
	m["epochMsec"] = epochMsec;
	if(epochMsec > 0)
		m["dateTime"] = chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(epochMsec);
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
