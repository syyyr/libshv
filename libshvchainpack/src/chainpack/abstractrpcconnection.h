#pragma once

#include "rpcmessage.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT AbstractRpcConnection
{
public:
	static constexpr int WAIT_FOREVER = -1;
	static constexpr int DEFAULT_RPC_TIMEOUT = 0;

	static constexpr int DEFAULT_RPC_BROKER_PORT = 3755;
public:
	virtual void close() = 0;
	virtual void abort() = 0;

	virtual void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) = 0;
	virtual RpcResponse sendMessageSync(const shv::chainpack::RpcRequest &rpc_request, int time_out_ms = DEFAULT_RPC_TIMEOUT) = 0;
	virtual void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) = 0;

	void sendNotify(std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	void sendShvNotify(const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	void sendResponse(const shv::chainpack::RpcValue &request_id, const shv::chainpack::RpcValue &result);
	void sendError(const shv::chainpack::RpcValue &request_id, const shv::chainpack::RpcResponse::Error &error);
	unsigned callMethod(std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	unsigned callShvMethod(const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	RpcResponse callMethodSync(const std::string &method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue(), int rpc_timeout = DEFAULT_RPC_TIMEOUT);
	RpcResponse callShvMethodSync(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue(), int rpc_timeout = DEFAULT_RPC_TIMEOUT);

	static int defaultRpcTimeout();
	static int setDefaultRpcTimeout(int rpc_timeout);
};

} // namespace chainpack
} // namespace shv
