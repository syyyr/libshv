#ifndef SHV_CORE_UTILS_SHVJOURNALENTRY_H
#define SHV_CORE_UTILS_SHVJOURNALENTRY_H

#include "../shvcoreglobal.h"

#include "shvlogtypeinfo.h"

#include <shv/chainpack/datachange.h>
#include <shv/chainpack/rpc.h>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvJournalEntry
{
public:
	//static const char *DOMAIN_VAL_CHANGE; /// see shv::chainpack::Rpc::SIG_VAL_CHANGED
	//static const char *DOMAIN_VAL_FASTCHANGE; /// see shv::chainpack::Rpc::SIG_VAL_FASTCHANGED
	//static const char *DOMAIN_VAL_SERVICECHANGE; /// see shv::chainpack::Rpc::SIG_SERVICE_VAL_CHANGED
	//static const char *DOMAIN_SHV_SYSTEM;
	//static const char *DOMAIN_SHV_COMMAND;

	static const char* PATH_APP_START;
	// snapshot begin-end cannot be implemented in the consistent way
	// for example: getLog() with since == log-file-beginning, have to parse log file to find snapshot-end
	// without this feature we can use just log reply even filtered
	//static const char* PATH_SNAPSHOT_BEGIN;
	//static const char* PATH_SNAPSHOT_END;
	static const char* PATH_DATA_MISSING;
	static const char* PATH_DATA_DIRTY;

	static const char* DATA_MISSING_UNAVAILABLE;
	static const char* DATA_MISSING_NOT_EXISTS;

	using SampleType = shv::chainpack::DataChange::SampleType;

	static constexpr int NO_SHORT_TIME = -1;

	int64_t epochMsec = 0;
	std::string path;
	shv::chainpack::RpcValue value;
	int shortTime = NO_SHORT_TIME;
	std::string domain;
	SampleType sampleType = SampleType::Continuous;
	std::string userId;

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
		: ShvJournalEntry(path, value, shv::chainpack::Rpc::SIG_VAL_CHANGED, NO_SHORT_TIME, SampleType::Continuous) {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, int short_time)
		: ShvJournalEntry(path, value, shv::chainpack::Rpc::SIG_VAL_FASTCHANGED, short_time, SampleType::Continuous) {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, std::string domain)
		: ShvJournalEntry(path, value, std::move(domain), NO_SHORT_TIME, SampleType::Continuous) {}

	bool isValid() const {return !path.empty() && epochMsec > 0;}
	bool operator==(const ShvJournalEntry &o) const
	{
		return epochMsec == o.epochMsec
				&& path == o.path
				&& value == o.value
				&& shortTime == o.shortTime
				&& domain == o.domain
				&& sampleType == o.sampleType
				&& userId == o.userId
				;
	}
	void setShortTime(int short_time) {shortTime = short_time;}
	shv::chainpack::RpcValue::DateTime dateTime() const { return shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(epochMsec); }
	shv::chainpack::RpcValue toRpcValueMap() const;
	shv::chainpack::DataChange toDataChange() const;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVJOURNALENTRY_H
