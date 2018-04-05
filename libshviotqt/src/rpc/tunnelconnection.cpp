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
		RPC_META_KEY_DEF(Host),
		RPC_META_KEY_DEF(Port),
		RPC_META_KEY_DEF(User),
		RPC_META_KEY_DEF(Password),
		RPC_META_KEY_DEF(ParentClientId),
		RPC_META_KEY_DEF(CallerClientIds),
		//RPC_META_KEY_DEF(TunName),
		//RPC_META_KEY_DEF(TunnelResponseRequestId),
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

chainpack::RpcValue TunnelParams::callerClientIds() const
{
	return value(MetaType::Key::CallerClientIds);
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
