#include "tunnelconnection.h"

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

TunnelAppCliOptions::TunnelAppCliOptions(QObject *parent)
	: Super(parent)
{
	//addOption("tunnel.parentClientId").setType(QVariant::String).setNames({"-t", "--tunnel-name"}).setComment(tr("Shv tree, where device should be mounted to. Only paths beginning with test/ are enabled. --mount-point version is deprecated"));

}

TunnelConnection::TunnelConnection(QObject *parent)
	: Super(SyncCalls::Disabled, parent)
{
	setConnectionType(cp::Rpc::TYPE_TUNNEL);
}

TunnelParams::MetaType::MetaType()
	: Super("TunnelParams")
{
	m_keys = {
		{(int)Key::Host, {(int)Key::Host, "Host"}},
		{(int)Key::Port, {(int)Key::Port, "Port"}},
		{(int)Key::User, {(int)Key::User, "User"}},
		{(int)Key::Password, {(int)Key::Password, "Password"}},
		{(int)Key::ParentClientId, {(int)Key::ParentClientId, "ParentClientId"}},
		{(int)Key::CallerClientIds, {(int)Key::CallerClientIds, "CallerClientIds"}},
		{(int)Key::TunName, {(int)Key::TunName, "TunName"}},
		{(int)Key::TunnelResponseRequestId, {(int)Key::TunnelResponseRequestId, "TunnelResponseRequestId"}},
	};
	//m_tags = {
	//	{(int)Tag::RequestId, {(int)Tag::RequestId, "id"}},
	//	{(int)Tag::ShvPath, {(int)Tag::ShvPath, "shvPath"}},
	//};
}

void TunnelParams::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

TunnelParams::TunnelParams()
	: Super()
{
	MetaType::registerMetaType();
}

TunnelParams::TunnelParams(const IMap &m)
	: Super(m)
{
	MetaType::registerMetaType();
}

chainpack::RpcValue TunnelParams::toRpcValue() const
{
	cp::RpcValue ret(*this);
	ret.setMetaValue(cp::meta::Tag::MetaTypeId, TunnelParams::MetaType::ID);
	return ret;
}

/*
void TunnelConnection::setCliOptions(const TunnelAppCliOptions *cli_opts)
{
	Super::setCliOptions(cli_opts);
	//setCheckBrokerConnectedInterval(0);
	if(cli_opts) {
		chainpack::RpcValue::Map opts = connectionOptions().toMap();
		cp::RpcValue::Map dev;
		dev["parentClientId"] = cli_opts->parentClientId().toInt();
		opts[cp::Rpc::TYPE_TUNNEL] = dev;
		setconnectionOptions(opts);
	}
}
*/

} // namespace rpc
} // namespace iotqt
} // namespace shv
