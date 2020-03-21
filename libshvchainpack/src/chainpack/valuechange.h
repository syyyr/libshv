#pragma once

#include "../shvchainpackglobal.h"

#include "rpcvalue.h"
#include "utils.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ValueChange
{
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::ValueChange};
		struct Tag { enum Enum {DateTime = chainpack::meta::Tag::USER,
								ShortTime,
								MAX};};
		//struct Key { enum Enum {Value = 1, DateTime, ShortTime, MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	ValueChange() {}
	ValueChange(const RpcValue &val, const RpcValue &date_time);
	ValueChange(const RpcValue &val, const RpcValue &date_time, const RpcValue &short_time);

	static bool isValueChange(const RpcValue &rv)
	{
		return rv.metaTypeId() == shv::chainpack::ValueChange::MetaType::ID
				&& rv.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID;
	}

	RpcValue value() const { return m_value; }
	void setValue(const RpcValue &val);

	bool hasDateTime() const { return m_dateTime.isDateTime(); }
	RpcValue dateTime() const { return m_dateTime.isDateTime()? m_dateTime: RpcValue(); }
	void setDateTime(const RpcValue &dt) { m_dateTime = dt.isDateTime()? dt: RpcValue(); }

	bool hasShortTime() const { return m_shortTime.isUInt(); }
	RpcValue shortTime() const { return m_shortTime.isUInt()? m_shortTime: RpcValue(); }
	void setShortTime(const RpcValue &st) { m_shortTime = st.isUInt()? st: RpcValue(); }

	static ValueChange fromRpcValue(const RpcValue &val);
	RpcValue toRpcValue() const;
private:
	RpcValue m_value;
	RpcValue m_dateTime;
	RpcValue m_shortTime;
};

} // namespace chainpack
} // namespace shv
