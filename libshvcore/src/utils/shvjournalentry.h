#ifndef SHV_CORE_UTILS_SHVJOURNALENTRY_H
#define SHV_CORE_UTILS_SHVJOURNALENTRY_H

#include "../shvcoreglobal.h"

#include "shvtypeinfo.h"

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
	static constexpr auto DOMAIN_VAL_CHANGE = shv::chainpack::Rpc::SIG_VAL_CHANGED;
	static constexpr auto DOMAIN_VAL_FASTCHANGE = shv::chainpack::Rpc::SIG_VAL_FASTCHANGED;
	static constexpr auto DOMAIN_VAL_SERVICECHANGE = shv::chainpack::Rpc::SIG_SERVICE_VAL_CHANGED;
	static constexpr auto DOMAIN_SHV_SYSTEM = "SHV_SYS";
	static constexpr auto DOMAIN_SHV_COMMAND = shv::chainpack::Rpc::SIG_COMMAND_LOGGED;


	static constexpr auto PATH_APP_START = "APP_START";
	// snapshot begin-end cannot be implemented in the consistent way
	// for example: getLog() with since == log-file-beginning, have to parse log file to find snapshot-end
	// without this feature we can use just log reply even filtered
	//static constexpr auto PATH_SNAPSHOT_BEGIN = "SNAPSHOT_BEGIN";
	//static constexpr auto PATH_SNAPSHOT_END = "SNAPSHOT_END";
	static constexpr auto PATH_DATA_MISSING = "DATA_MISSING";
	static constexpr auto PATH_DATA_DIRTY = "DATA_DIRTY";

	static constexpr auto DATA_MISSING_UNAVAILABLE = "Unavailable";
	static constexpr auto DATA_MISSING_NOT_EXISTS = "NotExists";


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

	ShvJournalEntry() = default;
	ShvJournalEntry(std::string path_, shv::chainpack::RpcValue value_, std::string domain_, int short_time, ValueFlags flags, int64_t epoch_msec = 0)
		: epochMsec(epoch_msec)
		, path(std::move(path_))
		, value{value_}
		, shortTime(short_time)
		, domain(std::move(domain_))
		, valueFlags(flags)
	{
	}
	ShvJournalEntry(std::string path_, shv::chainpack::RpcValue value_)
		: ShvJournalEntry(path_, value_, DOMAIN_VAL_CHANGE, NO_SHORT_TIME, NO_VALUE_FLAGS) {}
	ShvJournalEntry(std::string path_, shv::chainpack::RpcValue value_, int short_time)
		: ShvJournalEntry(path_, value_, DOMAIN_VAL_FASTCHANGE, short_time, NO_VALUE_FLAGS) {}
	ShvJournalEntry(std::string path_, shv::chainpack::RpcValue value_, std::string domain_)
		: ShvJournalEntry(path_, value_, std::move(domain_), NO_SHORT_TIME, NO_VALUE_FLAGS) {}

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
