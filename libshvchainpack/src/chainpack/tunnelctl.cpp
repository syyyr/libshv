#include "tunnelctl.h"

namespace shv {
namespace chainpack {

//================================================================
// TunnelCtl
//================================================================
TunnelCtl::MetaType::MetaType()
	: Super("TunnelCtl")
{
	m_keys = {
		RPC_META_KEY_DEF(State),
		RPC_META_KEY_DEF(Host),
		RPC_META_KEY_DEF(Port),
		RPC_META_KEY_DEF(Secret),
		RPC_META_KEY_DEF(RequestId),
		RPC_META_KEY_DEF(CallerIds),
	};
	//m_tags = {
	//	RPC_META_TAG_DEF(shvPath)
	//};
}

void TunnelCtl::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

TunnelCtl::TunnelCtl(State::Enum st)
 : Super(RpcValue::IMap())
{
	MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	setState(st);
}

TunnelCtl::TunnelCtl(const RpcValue::Map &o)
	: Super(o)
{
	MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
}

TunnelCtl::TunnelCtl(const RpcValue &o)
	: Super(o)
{
}

//================================================================
// FindTunnelResponse
//================================================================
FindTunnelResponse FindTunnelResponse::fromFindTunnelRequest(const FindTunnelRequest &rq)
{
	FindTunnelResponse ret;
	ret.setHost(rq.host());
	ret.setPort(rq.port());
	ret.setCallerIds(rq.callerIds());
	ret.setSecret(rq.secret());
	return ret;
}

//================================================================
// CreateTunnelRequest
//================================================================
CreateTunnelRequest CreateTunnelRequest::fromFindTunnelResponse(const FindTunnelResponse &resp)
{
	CreateTunnelRequest ret;
	ret.setHost(resp.host());
	ret.setPort(resp.port());
	ret.setSecret(resp.secret());
	ret.setCallerIds(resp.callerIds());
	return ret;
}

} // namespace iotqt
} // namespace shv
