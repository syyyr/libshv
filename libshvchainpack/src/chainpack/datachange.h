#pragma once

#include "../shvchainpackglobal.h"

#include "rpcvalue.h"
#include "utils.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT DataChange
{
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::DataChange};
		struct Tag { enum Enum {DateTime = chainpack::meta::Tag::USER,
								ShortTime,
								ValueFlags,
								SpecialListValue,
								MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	enum GetValueAgeOption {DONT_CARE_TS = -2, USE_CACHE = -1, RELOAD_FORCE, RELOAD_OLDER};
	static constexpr int NO_SHORT_TIME = -1;
	enum ValueFlag {Snapshot = 0, Spontaneous, ValueFlagCount};
	using ValueFlags = unsigned;
	static constexpr ValueFlags NO_VALUE_FLAGS = 0;
	static const char* valueFlagToString(ValueFlag flag);
	static std::string valueFlagsToString(ValueFlags st);

	DataChange() = default;
	DataChange(const RpcValue &val, const RpcValue::DateTime &date_time, int short_time = NO_SHORT_TIME);
	DataChange(const RpcValue &val, unsigned short_time);

	bool isValid() const { return m_value.isValid(); }
	static bool isDataChange(const RpcValue &rv);

	RpcValue value() const { return m_value; }
	void setValue(const RpcValue &val);

	bool hasDateTime() const { return m_dateTime.msecsSinceEpoch() > 0; }
	RpcValue dateTime() const { return hasDateTime()? RpcValue(m_dateTime): RpcValue(); }
	void setDateTime(const RpcValue &dt) { m_dateTime = dt.isDateTime()? dt.toDateTime(): RpcValue::DateTime(); }

	int64_t epochMSec() const { return m_dateTime.msecsSinceEpoch(); }

	bool hasShortTime() const { return m_shortTime > NO_SHORT_TIME; }
	RpcValue shortTime() const { return hasShortTime()? RpcValue(static_cast<unsigned>(m_shortTime)): RpcValue(); }
	void setShortTime(const RpcValue &st) { m_shortTime = (st.isUInt() || (st.isInt() && st.toInt() >= 0))? st.toInt(): NO_SHORT_TIME; }

	bool hasValueflags() const { return m_valueFlags != NO_VALUE_FLAGS; }
	void setValueFlags(ValueFlags st) { m_valueFlags = st; }
	ValueFlags valueFlags() const { return m_valueFlags; }

	bool isSpontaneous() const { return testBit(m_valueFlags, ValueFlag::Spontaneous); }
	void setSpontaneous(bool b) { setBit(m_valueFlags, ValueFlag::Spontaneous, b); }

	static bool testBit(const unsigned &n, int pos);
	static void setBit(unsigned &n, int pos, bool b);

	static DataChange fromRpcValue(const RpcValue &val);
	RpcValue toRpcValue() const;
private:
	RpcValue m_value;
	RpcValue::DateTime m_dateTime;
	int m_shortTime = NO_SHORT_TIME;
	ValueFlags m_valueFlags = NO_VALUE_FLAGS;
};

} // namespace chainpack
} // namespace shv
