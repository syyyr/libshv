#include "rpc.h"

namespace shv {
namespace chainpack {

const char* Rpc::METH_HELLO = "hello";
const char* Rpc::METH_LOGIN = "login";

const char* Rpc::METH_GET = "get";
const char* Rpc::METH_SET = "set";
const char* Rpc::METH_VAL_CHANGED = "chng";

const char* Rpc::JSONRPC_ID = "id";
const char* Rpc::JSONRPC_METHOD = "method";
const char* Rpc::JSONRPC_PARAMS = "params";
const char* Rpc::JSONRPC_RESULT = "result";
const char* Rpc::JSONRPC_ERROR = "error";
const char* Rpc::JSONRPC_SHV_PATH = "shvPath";
const char* Rpc::JSONRPC_CALLER_ID = "callerId";

const char *Rpc::ProtocolVersionToString(Rpc::ProtocolVersion pv)
{
	switch(pv) {
	case ProtocolVersion::ChainPack: return "ChainPack";
	case ProtocolVersion::Cpon: return "Cpon";
	case ProtocolVersion::JsonRpc: return "JsonRpc";
	case ProtocolVersion::Invalid: return "Invalid";
	}
	return "???";
}

} // namespace chainpack
} // namespace shv
