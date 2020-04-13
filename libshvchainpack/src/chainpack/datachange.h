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
								Domain,
								SampleType,
								MAX};};
		//struct Key { enum Enum {Value = 1, DateTime, ShortTime, MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	enum GetValueAgeOption {DONT_CARE_TS = -2, USE_CACHE = -1, RELOAD_FORCE, RELOAD_OLDER};
	static constexpr int NO_SHORT_TIME = -1;
	enum class SampleType : uint8_t {Invalid = 0, Continuous , Discrete};
	DataChange() {}
	DataChange(const RpcValue &val, const RpcValue::DateTime &date_time, int short_time = NO_SHORT_TIME);
	DataChange(const RpcValue &val, unsigned short_time);

	static bool isDataChange(const RpcValue &rv)
	{
		return rv.metaTypeId() == shv::chainpack::DataChange::MetaType::ID
				&& rv.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID;
	}

	RpcValue value() const { return m_value; }
	void setValue(const RpcValue &val);

	bool hasDateTime() const { return m_dateTime.msecsSinceEpoch() > 0; }
	RpcValue dateTime() const { return hasDateTime()? RpcValue(m_dateTime): RpcValue(); }
	void setDateTime(const RpcValue &dt) { m_dateTime = dt.isDateTime()? dt.toDateTime(): RpcValue::DateTime(); }

	bool hasShortTime() const { return m_shortTime > NO_SHORT_TIME; }
	RpcValue shortTime() const { return hasShortTime()? RpcValue((unsigned)m_shortTime): RpcValue(); }
	void setShortTime(const RpcValue &st) { m_shortTime = (st.isUInt() || (st.isInt() && st.toInt() >= 0))? st.toInt(): NO_SHORT_TIME; }

	void setDomain(const std::string &d) { m_domain = d; }
	const std::string& domain() const { return m_domain; }

	void setSampleType(SampleType st) { m_sampleType = st; }
	void setSampleType(int st) { m_sampleType = (st >= (int)SampleType::Invalid && st <= (int)SampleType::Discrete)? static_cast<SampleType>(st): SampleType::Invalid; }
	SampleType sampleType() const { return m_sampleType; }

	static DataChange fromRpcValue(const RpcValue &val);
	RpcValue toRpcValue() const;
private:
	RpcValue m_value;
	std::string m_domain;
	RpcValue::DateTime m_dateTime;
	int m_shortTime;
	SampleType m_sampleType = SampleType::Invalid;
};

} // namespace chainpack
} // namespace shv
