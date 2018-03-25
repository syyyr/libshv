#include "clientappclioptions.h"

#include <shv/chainpack/abstractrpcconnection.h>

namespace shv {
namespace iotqt {
namespace rpc {

ClientAppCliOptions::ClientAppCliOptions(QObject *parent)
	: Super(parent)
{
	//addOption("locale").setType(QVariant::String).setNames("--locale").setComment(tr("Application locale")).setDefaultValue("system");
	addOption("user.name").setType(QVariant::String).setNames("-u", "--user").setComment(tr("User name"));
	addOption("server.host").setType(QVariant::String).setNames("-s", "--server-host").setComment(tr("server host")).setDefaultValue("localhost");
	addOption("server.port").setType(QVariant::Int).setNames("-p", "--server-port").setComment(tr("server port")).setDefaultValue(shv::chainpack::AbstractRpcConnection::DEFAULT_RPC_BROKER_PORT);
	addOption("rpc.timeout").setType(QVariant::Int).setNames("--rpc-timeout").setComment(tr("RPC timeout msec")).setDefaultValue(shv::chainpack::AbstractRpcConnection::DEFAULT_RPC_TIMEOUT);
	addOption("rpc.metaTypeExplicit").setType(QVariant::Bool).setNames("--rpc-metatype-explicit").setComment(tr("RpcMessage Type ID is included in RpcMessage when set, for more verbose -v rpcmsg log output")).setDefaultValue(false);
}

} // namespace client
} // namespace iot
} // namespace shv
