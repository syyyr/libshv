#pragma once

#include "rpcmessage.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT IRpcConnection
{
public:
	static constexpr int DEFAULT_RPC_BROKER_PORT = 3755;

	enum class LoginType {Invalid = 0, Plain, Sha1, RsaOaep};
public:
	IRpcConnection();
	virtual ~IRpcConnection();

	virtual int connectionId() const {return m_connectionId;}

	virtual void close() = 0;
	virtual void abort() = 0;

	virtual void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) = 0;
	//virtual RpcResponse sendMessageSync(const shv::chainpack::RpcRequest &rpc_request, int time_out_ms = DEFAULT_RPC_TIMEOUT) = 0;
	virtual void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) = 0;

	int nextRequestId();

	void sendNotify(std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	void sendShvNotify(const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	void sendResponse(const shv::chainpack::RpcValue &request_id, const shv::chainpack::RpcValue &result);
	void sendError(const shv::chainpack::RpcValue &request_id, const shv::chainpack::RpcResponse::Error &error);
	int callMethod(const shv::chainpack::RpcRequest &rq);
	int callMethod(std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	int callShvMethod(const std::string &shv_path, std::string method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue(), const RpcValue &grant = shv::chainpack::RpcValue());
	//RpcResponse callMethodSync(const std::string &method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue(), int rpc_timeout = DEFAULT_RPC_TIMEOUT);
	//RpcResponse callShvMethodSync(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue(), int rpc_timeout = DEFAULT_RPC_TIMEOUT);
	int callMethodSubscribe(const std::string &shv_path, std::string method, const RpcValue &grant = shv::chainpack::RpcValue());
	int callMethodUnsubscribe(const std::string &shv_path, std::string method, const RpcValue &grant = shv::chainpack::RpcValue());

	static std::string loginTypeToString(LoginType t);
	static LoginType loginTypeFromString(const std::string &s);
protected:
	static int nextConnectionId();
protected:
	int m_connectionId;
};

} // namespace chainpack
} // namespace shv
