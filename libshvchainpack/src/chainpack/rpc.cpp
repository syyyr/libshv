#include "rpc.h"

namespace shv {
namespace chainpack {

const char* Rpc::OPT_IDLE_WD_TIMEOUT = "idleWatchDogTimeOut";

const char* Rpc::KEY_CONNECTION_OPTIONS = "options";
const char* Rpc::KEY_CONNECTION_TYPE = "type";
const char* Rpc::KEY_CLIENT_ID = "clientId";
const char* Rpc::KEY_MOUT_POINT = "mountPoint";
const char* Rpc::KEY_RELATIVE_PATH = "relPath";
const char* Rpc::KEY_DEVICE_ID = "deviceId";
//const char* Rpc::KEY_TUNNEL_HANDLE = "tunnelHandle";

const char* Rpc::TYPE_CLIENT = "client";
const char* Rpc::TYPE_DEVICE = "device";
//const char* Rpc::TYPE_TUNNEL = "tunnel";

const char* Rpc::METH_HELLO = "hello";
const char* Rpc::METH_LOGIN = "login";

const char* Rpc::METH_GET = "get";
const char* Rpc::METH_SET = "set";
const char* Rpc::METH_DIR = "dir";
const char* Rpc::METH_LS = "ls";
const char* Rpc::METH_PING = "ping";
const char* Rpc::METH_ECHO = "echo";
const char* Rpc::METH_APP_NAME = "appName";
const char* Rpc::METH_DEVICE_ID = "deviceId";
const char* Rpc::METH_MOUNT_POINT = "mountPoint";
const char* Rpc::METH_CONNECTION_TYPE = "connectionType";
const char* Rpc::METH_SUBSCRIBE = "subscribe";
const char* Rpc::METH_RUN_CMD = "runCmd";
const char* Rpc::METH_LAUNCH_REXEC = "launchRexec";
const char* Rpc::METH_HELP = "help";

const char* Rpc::PAR_PATH = "path";
const char* Rpc::PAR_METHOD = "method";

const char* Rpc::NTF_VAL_CHANGED = "chng";
//const char* Rpc::NTF_CONNECTED = "connected";
//const char* Rpc::NTF_DISCONNECTED = "disconnected";
const char* Rpc::NTF_MOUNTED_CHANGED = "mntchng";
const char* Rpc::NTF_CONNECTED_CHANGED = "connchng";

const char* Rpc::DIR_BROKER = ".broker";
const char* Rpc::DIR_BROKER_APP = ".broker/app";
const char* Rpc::DIR_CLIENTS = "clients";

const char* Rpc::JSONRPC_REQUEST_ID = "id";
const char* Rpc::JSONRPC_METHOD = "method";
const char* Rpc::JSONRPC_PARAMS = "params";
const char* Rpc::JSONRPC_RESULT = "result";
const char* Rpc::JSONRPC_ERROR = "error";
const char* Rpc::JSONRPC_ERROR_CODE = "code";
const char* Rpc::JSONRPC_ERROR_MESSAGE = "message";
const char* Rpc::JSONRPC_SHV_PATH = "path";
const char* Rpc::JSONRPC_CALLER_ID = "cid";

const char *Rpc::protocolTypeToString(Rpc::ProtocolType pv)
{
	switch(pv) {
	case ProtocolType::ChainPack: return "ChainPack";
	case ProtocolType::Cpon: return "Cpon";
	case ProtocolType::JsonRpc: return "JsonRpc";
	case ProtocolType::Invalid: return "Invalid";
	}
	return "???";
}

} // namespace chainpack
} // namespace shv
