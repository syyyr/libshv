#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT TunnelHandle : public shv::chainpack::RpcValue::IMap
{
	using Super = shv::chainpack::RpcValue::IMap;
public:
	class  MetaType : public shv::chainpack::meta::MetaType
	{
		using Super = shv::chainpack::meta::MetaType;
	public:
		enum {ID = shv::chainpack::meta::GlobalNS::RegisteredMetaTypes::RpcTunnelHandle};
		//struct Tag { enum Enum {RequestId = meta::Tag::USER,
		//						MAX};};
		struct Key { enum Enum {CallerClientIds = 1, TunnelClientId, MAX};};

		MetaType();
		static void registerMetaType();
	};
public:
	TunnelHandle();
	TunnelHandle(const shv::chainpack::RpcValue::IMap &m);
	TunnelHandle(const shv::chainpack::RpcValue &caller_ids, unsigned tun_id);

	shv::chainpack::RpcValue toRpcValue() const;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv

