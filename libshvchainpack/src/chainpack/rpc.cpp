#include "rpc.h"

namespace shv {
namespace chainpack {

const char* Rpc::TYPE_CLIENT = "client";
const char* Rpc::TYPE_DEVICE = "device";
const char* Rpc::TYPE_TUNNEL = "tunnel";

const char* Rpc::METH_HELLO = "hello";
const char* Rpc::METH_LOGIN = "login";

const char* Rpc::METH_GET = "get";
const char* Rpc::METH_SET = "set";
const char* Rpc::METH_VAL_CHANGED = "chng";
const char* Rpc::METH_DIR = "dir";
const char* Rpc::METH_LS = "ls";
const char* Rpc::METH_PING = "ping";
const char* Rpc::METH_ECHO = "echo";

const char* Rpc::JSONRPC_ID = "id";
const char* Rpc::JSONRPC_METHOD = "method";
const char* Rpc::JSONRPC_PARAMS = "params";
const char* Rpc::JSONRPC_RESULT = "result";
const char* Rpc::JSONRPC_ERROR = "error";
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

} // namespace chainpack
} // namespace shv
