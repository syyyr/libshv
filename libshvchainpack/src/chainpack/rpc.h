#pragma once

#include "../shvchainpackglobal.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT Rpc
{
public:
	enum class ProtocolType {Invalid = 0, ChainPack, Cpon, JsonRpc};
	static const char* ProtocolTypeToString(ProtocolType pv);

	static const char* TYPE_CLIENT;
	static const char* TYPE_DEVICE;
	static const char* TYPE_TUNNEL;

	static const char* METH_HELLO;
	static const char* METH_LOGIN;
	static const char* METH_GET;
	static const char* METH_SET;
	static const char* METH_VAL_CHANGED;
	static const char* METH_DIR;
	static const char* METH_LS;
	static const char* METH_PING;

	static const char* JSONRPC_ID;
	static const char* JSONRPC_METHOD;
	static const char* JSONRPC_PARAMS;
	static const char* JSONRPC_RESULT;
	static const char* JSONRPC_ERROR;
	static const char* JSONRPC_SHV_PATH;
	static const char* JSONRPC_CALLER_ID;
};

} // namespace chainpack
} // namespace shv
