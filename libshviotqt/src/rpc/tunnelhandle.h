#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcmessage.h>

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
		struct Key {
			enum Enum {
				RequestId = shv::chainpack::RpcMessage::MetaType::Tag::RequestId,
				CallerIds = shv::chainpack::RpcMessage::MetaType::Tag::CallerIds,
			};
		};

		MetaType();
		static void registerMetaType();
	};
public:
	TunnelHandle();
	TunnelHandle(const shv::chainpack::RpcValue::IMap &m);
	TunnelHandle(const shv::chainpack::RpcValue &request_id, const shv::chainpack::RpcValue &caller_ids);

	shv::chainpack::RpcValue requestId() const {return value(MetaType::Key::RequestId);}
	shv::chainpack::RpcValue callerIds() const {return value(MetaType::Key::CallerIds);}

	shv::chainpack::RpcValue toRpcValue() const;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv

