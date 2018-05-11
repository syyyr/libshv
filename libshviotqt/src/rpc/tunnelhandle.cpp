#include "tunnelhandle.h"

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

TunnelHandle::MetaType::MetaType()
	: Super("TunnelHandle")
{
	m_keys = {
		RPC_META_KEY_DEF(TunnelRelativePath),
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

TunnelHandle::TunnelHandle(const std::string &tunnel_relative_path)
	:TunnelHandle()
{
	(*this)[MetaType::Key::TunnelRelativePath] = tunnel_relative_path;
}

chainpack::RpcValue TunnelHandle::toRpcValue() const
{
	cp::RpcValue ret(*this);
	ret.setMetaValue(cp::meta::Tag::MetaTypeId, TunnelHandle::MetaType::ID);
	return ret;
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
