#include "rpc.h"

namespace shv {
namespace chainpack {

const char* Rpc::OPT_IDLE_WD_TIMEOUT = "idleWatchDogTimeOut";

const char* Rpc::KEY_CLIENT_ID = "clientId";
const char* Rpc::KEY_MOUT_POINT = "mountPoint";
const char* Rpc::KEY_DEVICE_ID = "deviceId";
const char* Rpc::KEY_TUNNEL_HANDLE = "tunnelHandle";

const char* Rpc::TYPE_CLIENT = "client";
const char* Rpc::TYPE_DEVICE = "device";
const char* Rpc::TYPE_TUNNEL = "tunnel";

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
const char* Rpc::METH_CONNECTION_TYPE = "connectionType";
const char* Rpc::METH_SUBSCRIBE = "subscribe";

const char* Rpc::PAR_PATH = "path";
const char* Rpc::PAR_METHOD = "method";

const char* Rpc::NTF_VAL_CHANGED = "chng";
const char* Rpc::NTF_CONNECTED = "connected";
const char* Rpc::NTF_DISCONNECTED = "disconnected";

const char* Rpc::DIR_BROKER = ".broker";
const char* Rpc::DIR_BROKER_APP = ".broker/app";

const char* Rpc::JSONRPC_ID = "id";
const char* Rpc::JSONRPC_METHOD = "method";
const char* Rpc::JSONRPC_PARAMS = "params";
const char* Rpc::JSONRPC_RESULT = "result";
const char* Rpc::JSONRPC_ERROR = "error";
const char* Rpc::JSONRPC_ERROR_CODE = "code";
const char* Rpc::JSONRPC_ERROR_MESSAGE = "message";
const char* Rpc::JSONRPC_SHV_PATH = "shvPath";
const char* Rpc::JSONRPC_CALLER_ID = "callerId";

const char *Rpc::ProtocolTypeToString(Rpc::ProtocolType pv)
{
	switch(pv) {
	case ProtocolType::ChainPack: return "ChainPack";
	case ProtocolType::Cpon: return "Cpon";
	case ProtocolType::JsonRpc: return "JsonRpc";
	case ProtocolType::Invalid: return "Invalid";
	}
	return "???";
}

std::string Rpc::joinShvPath(const std::string &p1, const std::string &p2)
{
	if(p2.empty())
		return p1;
	if(p1.empty())
		return p2;
	std::string ret = p1;
	if(p1[p1.length()  -1] != '/' && p2[0] != '/')
		ret += '/';
	ret += p2;
	return ret;
}

} // namespace chainpack
} // namespace shv
