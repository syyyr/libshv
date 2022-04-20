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
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::ShvJournalEntry};
		MetaType();
		static void registerMetaType();
	};
public:
	static const char *DOMAIN_VAL_CHANGE;
	static const char *DOMAIN_VAL_FASTCHANGE;
	static const char *DOMAIN_VAL_SERVICECHANGE;
	static const char *DOMAIN_SHV_SYSTEM;
	static const char *DOMAIN_SHV_COMMAND;

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

	using ValueFlag = shv::chainpack::DataChange::ValueFlag;
	using ValueFlags = shv::chainpack::DataChange::ValueFlags;

	static constexpr int NO_SHORT_TIME = shv::chainpack::DataChange::NO_SHORT_TIME;
	static constexpr ValueFlags NO_VALUE_FLAGS = shv::chainpack::DataChange::NO_VALUE_FLAGS;

	int64_t epochMsec = 0;
	std::string path;
	shv::chainpack::RpcValue value;
	int shortTime = NO_SHORT_TIME;
	std::string domain;
	unsigned valueFlags = 0;
	std::string userId;

	ShvJournalEntry() {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, std::string domain, int short_time, ValueFlags flags, int64_t epoch_msec = 0)
		: epochMsec(epoch_msec)
		, path(std::move(path))
		, value{value}
		, shortTime(short_time)
		, domain(std::move(domain))
		, valueFlags(flags)
	{
	}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value)
		: ShvJournalEntry(path, value, DOMAIN_VAL_CHANGE, NO_SHORT_TIME, NO_VALUE_FLAGS) {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, int short_time)
		: ShvJournalEntry(path, value, DOMAIN_VAL_FASTCHANGE, short_time, NO_VALUE_FLAGS) {}
	ShvJournalEntry(std::string path, shv::chainpack::RpcValue value, std::string domain)
		: ShvJournalEntry(path, value, std::move(domain), NO_SHORT_TIME, NO_VALUE_FLAGS) {}

	bool isValid() const {return !path.empty() && epochMsec > 0;}
	bool isSpontaneous() const { return shv::chainpack::DataChange::testBit(valueFlags, ValueFlag::Spontaneous); }
	void setSpontaneous(bool b) { shv::chainpack::DataChange::setBit(valueFlags, ValueFlag::Spontaneous, b); }
	bool isSnapshotValue() const { return shv::chainpack::DataChange::testBit(valueFlags, ValueFlag::Snapshot); }
	void setSnapshotValue(bool b) { shv::chainpack::DataChange::setBit(valueFlags, ValueFlag::Snapshot, b); }
	bool operator==(const ShvJournalEntry &o) const
	{
		return epochMsec == o.epochMsec
				&& path == o.path
				&& value == o.value
				&& shortTime == o.shortTime
				&& domain == o.domain
				&& valueFlags == o.valueFlags
				&& userId == o.userId
				;
	}
	void setShortTime(int short_time) {shortTime = short_time;}
	shv::chainpack::RpcValue::DateTime dateTime() const { return shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(epochMsec); }
	shv::chainpack::RpcValue toRpcValueMap() const;

	static bool isShvJournalEntry(const shv::chainpack::RpcValue &rv);
	shv::chainpack::RpcValue toRpcValue() const;
	static ShvJournalEntry fromRpcValue(const shv::chainpack::RpcValue &rv);
	static ShvJournalEntry fromRpcValueMap(const shv::chainpack::RpcValue::Map &m);

	shv::chainpack::DataChange toDataChange() const;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVJOURNALENTRY_H
