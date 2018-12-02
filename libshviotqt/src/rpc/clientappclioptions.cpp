#include "clientappclioptions.h"

#include <shv/chainpack/abstractrpcconnection.h>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

ClientAppCliOptions::ClientAppCliOptions()
{
	//addOption("locale").setType(cp::RpcValue::Type::String).setNames("--locale").setComment(tr("Application locale")).setDefaultValue("system");
	addOption("login.user").setType(cp::RpcValue::Type::String).setNames("-u", "--user").setComment("Login user");
	addOption("login.password").setType(cp::RpcValue::Type::String).setNames("--password").setComment("Login password");
	addOption("login.passwordFile").setType(cp::RpcValue::Type::String).setNames("--password-file").setComment("Login password file");
	addOption("login.type").setType(cp::RpcValue::Type::String).setNames("--lt", "--login-type").setComment("Login type [PLAIN | SHA1 | RSAOAEP]").setDefaultValue("SHA1");
	addOption("server.host").setType(cp::RpcValue::Type::String).setNames("-s", "--server-host").setComment("server host").setDefaultValue("localhost");
	addOption("server.port").setType(cp::RpcValue::Type::Int).setNames("-p", "--server-port").setComment("server port").setDefaultValue(shv::chainpack::AbstractRpcConnection::DEFAULT_RPC_BROKER_PORT);

	addOption("rpc.protocolType").setType(cp::RpcValue::Type::String).setNames("--protocol-type").setComment("Protocol type [chainpack | cpon | jsonrpc]").setDefaultValue("chainpack");
	//addOption("rpc.timeout").setType(cp::RpcValue::Type::Int).setNames("--rpc-timeout").setComment(tr("RPC timeout msec")).setDefaultValue(shv::chainpack::AbstractRpcConnection::DEFAULT_RPC_TIMEOUT);
	//addOption("rpc.metaTypeExplicit").setType(cp::RpcValue::Type::Bool).setNames("--mtid", "--rpc-metatype-explicit").setComment("RpcMessage Type ID is included in RpcMessage when set, for more verbose -v rpcmsg log output").setDefaultValue(false);
	addOption("rpc.reconnectInterval").setType(cp::RpcValue::Type::Int).setNames("--rci", "--rpc-reconnect-interval").setComment("Reconnect to broker if connection lost at least after recoonect-interval seconds. Disabled when set to 0").setDefaultValue(10);
	addOption("rpc.heartbeatInterval").setType(cp::RpcValue::Type::Int).setNames("--hbi", "--rpc-heartbeat-interval").setComment("Send heart beat to broker every n sec. Disabled when set to 0").setDefaultValue(60);
}

} // namespace client
} // namespace iot
} // namespace shv
