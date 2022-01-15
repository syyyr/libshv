#include "rpc.h"

namespace shv {
namespace chainpack {

const char* Rpc::OPT_IDLE_WD_TIMEOUT = "idleWatchDogTimeOut";

const char* Rpc::KEY_OPTIONS = "options";
const char* Rpc::KEY_CLIENT_ID = "clientId";
const char* Rpc::KEY_MOUT_POINT = "mountPoint";
const char* Rpc::KEY_SHV_PATH = "shvPath";
const char* Rpc::KEY_DEVICE_ID = "deviceId";
const char* Rpc::KEY_DEVICE = "device";
const char* Rpc::KEY_BROKER = "broker";
const char* Rpc::KEY_TUNNEL = "tunnel";
const char* Rpc::KEY_LOGIN = "login";
const char* Rpc::KEY_SECRET = "secret";
const char* Rpc::KEY_SHV_USER = "shvUser";
const char* Rpc::KEY_BROKER_ID = "brokerId";

const char* Rpc::METH_HELLO = "hello";
const char* Rpc::METH_LOGIN = Rpc::KEY_LOGIN;

const char* Rpc::METH_GET = "get";
const char* Rpc::METH_SET = "set";
const char* Rpc::METH_DIR = "dir";
const char* Rpc::METH_LS = "ls";
const char* Rpc::METH_PING = "ping";
const char* Rpc::METH_ECHO = "echo";
const char* Rpc::METH_APP_NAME = "appName";
const char* Rpc::METH_APP_VERSION = "appVersion";
const char* Rpc::METH_GIT_COMMIT = "gitCommit";
const char* Rpc::METH_DEVICE_ID = "deviceId";
const char* Rpc::METH_DEVICE_TYPE = "deviceType";
const char* Rpc::METH_CLIENT_ID = "clientId";
const char* Rpc::METH_MOUNT_POINT = "mountPoint";
const char* Rpc::METH_SUBSCRIBE = "subscribe";
const char* Rpc::METH_UNSUBSCRIBE = "unsubscribe";
const char* Rpc::METH_REJECT_NOT_SUBSCRIBED = "rejectNotSubscribed";
const char* Rpc::METH_RUN_CMD = "runCmd";
const char* Rpc::METH_RUN_SCRIPT = "runScript";
const char* Rpc::METH_LAUNCH_REXEC = "launchRexec";
const char* Rpc::METH_HELP = "help";
const char* Rpc::METH_GET_LOG = "getLog";

const char* Rpc::PAR_PATH = "path";
const char* Rpc::PAR_METHOD = "method";
const char* Rpc::PAR_PARAMS = "params";

const char* Rpc::SIG_VAL_CHANGED = "chng";
const char* Rpc::SIG_VAL_FASTCHANGED = "fastchng";
const char* Rpc::SIG_SERVICE_VAL_CHANGED = "svcchng";
const char* Rpc::SIG_MOUNTED_CHANGED = "mntchng";
const char* Rpc::SIG_COMMAND_LOGGED = "cmdlog";

const char* Rpc::ROLE_BROWSE = "bws";
const char* Rpc::ROLE_READ = "rd";
const char* Rpc::ROLE_WRITE = "wr";
const char* Rpc::ROLE_COMMAND = "cmd";
const char* Rpc::ROLE_CONFIG = "cfg";
const char* Rpc::ROLE_SERVICE = "srv";
const char* Rpc::ROLE_SUPER_SERVICE = "ssrv";
const char* Rpc::ROLE_DEVEL = "dev";
const char* Rpc::ROLE_ADMIN = "su";

const char* Rpc::ROLE_MASTER_BROKER = "masterBroker";

const char* Rpc::DIR_BROKER = ".broker";
const char* Rpc::DIR_BROKER_APP = ".broker/app";
const char* Rpc::DIR_CLIENTS = "clients";
const char* Rpc::DIR_MASTERS = "masters";

const char* Rpc::JSONRPC_REQUEST_ID = "id";
const char* Rpc::JSONRPC_METHOD = PAR_METHOD;
const char* Rpc::JSONRPC_PARAMS = PAR_PARAMS;
const char* Rpc::JSONRPC_RESULT = "result";
const char* Rpc::JSONRPC_ERROR = "error";
const char* Rpc::JSONRPC_ERROR_CODE = "code";
const char* Rpc::JSONRPC_ERROR_MESSAGE = "message";
const char* Rpc::JSONRPC_SHV_PATH = PAR_PATH;
const char* Rpc::JSONRPC_CALLER_ID = "cid";
const char* Rpc::JSONRPC_REV_CALLER_ID = "rcid";

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
