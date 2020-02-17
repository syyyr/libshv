#ifndef SHV_CORE_UTILS_SHVJOURNALENTRY_H
#define SHV_CORE_UTILS_SHVJOURNALENTRY_H

#include "../shvcoreglobal.h"

#include "shvlogtypeinfo.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvJournalEntry
{
public:
	static const char *DOMAIN_VAL_CHANGE; /// see shv::chainpack::Rpc::SIG_VAL_CHANGED
	static const char *DOMAIN_VAL_FASTCHANGE; /// see shv::chainpack::Rpc::SIG_VAL_FASTCHANGED
	static const char *DOMAIN_VAL_SERVICECHANGE; /// see shv::chainpack::Rpc::SIG_SERVICE_VAL_CHANGED
	static const char *DOMAIN_SHV_SYSTEM;

	static const char* SNAPSHOT_END;
	static const char* PATH_DATA_MISSING;
	static const char* DATA_MISSING_DEVICE_DISCONNECTED;
	static const char* DATA_MISSING_HP_DISCONNECTED;
	static const char* DATA_MISSING_LOG_FILE_CORRUPTED;
	static const char* DATA_MISSING_LOG_FILE_MISSING;
	static const char* DATA_MISSING_APP_SHUTDOWN;

	using SampleType = ShvLogTypeDescr::SampleType;

	static constexpr int NO_SHORT_TIME = -1;

	int64_t epochMsec = 0;
	std::string path;
	shv::chainpack::RpcValue value;
	int shortTime = NO_SHORT_TIME;
	std::string domain;
	SampleType sampleType = SampleType::Continuous;

	ShvJournalEntry() {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, std::string domain, int short_time, SampleType sample_type, int64_t epoch_msec = 0)
		: epochMsec(epoch_msec)
		, path(std::move(path))
		, value{value}
		, shortTime(short_time)
		, domain(std::move(domain))
		, sampleType(sample_type)
	{
	}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value)
		: ShvJournalEntry(path, value, DOMAIN_VAL_CHANGE, NO_SHORT_TIME, SampleType::Continuous) {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, int short_time)
		: ShvJournalEntry(path, value, DOMAIN_VAL_FASTCHANGE, short_time, SampleType::Continuous) {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, std::string domain)
		: ShvJournalEntry(path, value, std::move(domain), NO_SHORT_TIME, SampleType::Continuous) {}

	bool isValid() const {return !path.empty() && value.isValid();}
	bool operator==(const ShvJournalEntry &o) const
	{
		return epochMsec == o.epochMsec
				&& path == o.path
				&& value == o.value
				&& shortTime == o.shortTime
				&& domain == o.domain
				&& sampleType == o.sampleType;
	}
	void setShortTime(int short_time) {shortTime = short_time;}
	shv::chainpack::RpcValue::DateTime dateTime() const { return shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(epochMsec); }
	shv::chainpack::RpcValue toRpcValueMap() const;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVJOURNALENTRY_H
