#include "tunnelhandle.h"

namespace shv {
namespace iotqt {
namespace rpc {

TunnelHandle::MetaType::MetaType()
	: Super("TunnelHandle")
{
	m_keys = {
		RPC_META_KEY_DEF(TunnelHandle),
		RPC_META_KEY_DEF(TunnelClientId),
	};
	//m_tags = {
	//	{(int)Tag::RequestId, {(int)Tag::RequestId, "id"}},
	//	{(int)Tag::ShvPath, {(int)Tag::ShvPath, "shvPath"}},
	//};
}

void TunnelHandle::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

TunnelHandle::TunnelHandle()
	: Super()
{
	MetaType::registerMetaType();
}

TunnelHandle::TunnelHandle(const chainpack::RpcValue::IMap &m)
	: Super(m)
{
	MetaType::registerMetaType();
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
