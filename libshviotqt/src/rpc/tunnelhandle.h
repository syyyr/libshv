#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcmessage.h>

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT TunnelHandle
{
public:
	/*
	class  MetaType : public shv::chainpack::meta::MetaType
	{
		using Super = shv::chainpack::meta::MetaType;
	public:
		enum {ID = shv::chainpack::meta::GlobalNS::RegisteredMetaTypes::RpcTunnelHandle};
		//struct Tag { enum Enum {RequestId = meta::Tag::USER,
		//						MAX};};
		struct Key {
			enum Enum {
				RevRequestId = shv::chainpack::RpcMessage::MetaType::Tag::RequestId,
				CallerIds = shv::chainpack::RpcMessage::MetaType::Tag::CallerIds,
			};
		};

		MetaType();
		static void registerMetaType();
	};
	*/
public:
	TunnelHandle() {}
	TunnelHandle(unsigned req_id, const shv::chainpack::RpcValue &caller_ids)
		: m_requestId(req_id)
		, m_callerIds(caller_ids)
	{}

	unsigned requestId() const {return m_requestId;}
	const shv::chainpack::RpcValue& callerIds() const {return m_callerIds;}
	bool isValid() const {return requestId() > 0 && m_callerIds.isValid();}

	//shv::chainpack::RpcValue toRpcValue() const;
private:
	unsigned m_requestId = 0;
	shv::chainpack::RpcValue m_callerIds;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv

