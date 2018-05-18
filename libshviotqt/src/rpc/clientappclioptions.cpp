#include "clientappclioptions.h"

#include <shv/chainpack/abstractrpcconnection.h>

namespace shv {
namespace iotqt {
namespace rpc {

ClientAppCliOptions::ClientAppCliOptions(QObject *parent)
	: Super(parent)
{
	//addOption("locale").setType(QVariant::String).setNames("--locale").setComment(tr("Application locale")).setDefaultValue("system");
	addOption("login.user").setType(QVariant::String).setNames("-u", "--user").setComment(tr("Login user"));
	addOption("login.password").setType(QVariant::String).setNames("--password").setComment(tr("Login password"));
	addOption("server.host").setType(QVariant::String).setNames("-s", "--server-host").setComment(tr("server host")).setDefaultValue("localhost");
	addOption("server.port").setType(QVariant::Int).setNames("-p", "--server-port").setComment(tr("server port")).setDefaultValue(shv::chainpack::AbstractRpcConnection::DEFAULT_RPC_BROKER_PORT);

	addOption("rpc.protocolType").setType(QVariant::String).setNames("--protocol-type").setComment(tr("Protocol type [chainpack | cpon | jsonrpc]")).setDefaultValue("chainpack");
	addOption("rpc.timeout").setType(QVariant::Int).setNames("--rpc-timeout").setComment(tr("RPC timeout msec")).setDefaultValue(shv::chainpack::AbstractRpcConnection::DEFAULT_RPC_TIMEOUT);
	addOption("rpc.metaTypeExplicit").setType(QVariant::Bool).setNames("--mtid", "--rpc-metatype-explicit").setComment(tr("RpcMessage Type ID is included in RpcMessage when set, for more verbose -v rpcmsg log output")).setDefaultValue(false);
	addOption("rpc.reconnectInterval").setType(QVariant::Int).setNames({"--rci", "--rpc-reconnect-interval"}).setComment(tr("Reconnect to broker if connection lost at least after recoonect-interval seconds. Disabled when set to 0")).setDefaultValue(10);
	addOption("rpc.heartbeatInterval").setType(QVariant::Int).setNames({"--hbi", "--rpc-heartbeat-interval"}).setComment(tr("Send heart beat to broker every n sec. Disabled when set to 0")).setDefaultValue(60);

	//addOption("shv.sessionToken").setType(QVariant::String).setNames("-st", "--session-token")
	//		.setComment(tr("Session token containing authorization for this instance on the broker when connected."));
}

} // namespace client
} // namespace iot
} // namespace shv
