#include "tunnelhandle.h"

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {
#if 0
TunnelHandle::MetaType::MetaType()
	: Super("TunnelHandle")
{
	m_keys = {
		RPC_META_KEY_DEF(RequestId),
		RPC_META_KEY_DEF(CallerIds),
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

TunnelHandle::TunnelHandle(const cp::RpcValue::IMap &m)
	: Super(m)
{
	MetaType::registerMetaType();
}

TunnelHandle::TunnelHandle(const chainpack::RpcValue &request_id, const chainpack::RpcValue &caller_ids)
{
	(*this)[MetaType::Key::RevRequestId] = request_id;
	(*this)[MetaType::Key::CallerIds] = caller_ids;
}
chainpack::RpcValue TunnelHandle::toRpcValue() const
{
	return revRequestId();
}
#endif

} // namespace rpc
} // namespace iotqt
} // namespace shv
