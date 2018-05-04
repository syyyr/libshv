#include "abstractrpcconnection.h"
#include "exception.h"

namespace shv {
namespace chainpack {

void AbstractRpcConnection::sendNotify(std::string method, const RpcValue &params)
{
	sendShvNotify(std::string(), std::move(method), params);
}

void AbstractRpcConnection::sendShvNotify(const std::string &shv_path, std::string method, const RpcValue &params)
{
	RpcRequest rq;
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	rq.setMethod(std::move(method));
	rq.setParams(params);
	sendMessage(rq);
}

void AbstractRpcConnection::sendResponse(const RpcValue &request_id, const RpcValue &result)
{
	RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setResult(result);
	sendMessage(resp);
}

void AbstractRpcConnection::sendError(const RpcValue &request_id, const RpcResponse::Error &error)
{
	RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setError(error);
	sendMessage(resp);
}

static unsigned next_rqid()
{
	static unsigned n = 0;
	return ++n;
}

unsigned AbstractRpcConnection::callMethod(std::string method, const RpcValue &params)
{
	return callShvMethod(std::string(), std::move(method), params);
}

unsigned AbstractRpcConnection::callShvMethod(const std::string &shv_path, std::string method, const RpcValue &params)
{
	unsigned id = next_rqid();
	RpcRequest rq;
	rq.setRequestId(id);
	rq.setMethod(std::move(method));
	if(params.isValid())
		rq.setParams(params);
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	sendMessage(rq);
	return id;
}

RpcResponse AbstractRpcConnection::callMethodSync(const std::string &method, const RpcValue &params, int rpc_timeout)
{
	return callShvMethodSync(std::string(), method, params, rpc_timeout);
}

RpcResponse AbstractRpcConnection::callShvMethodSync(const std::string &shv_path, const std::string &method, const RpcValue &params, int rpc_timeout)
{
	RpcRequest rq;
	rq.setRequestId(next_rqid());
	rq.setMethod(method);
	rq.setParams(params);
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	//logRpc() << "--> sync method call:" << id << method;
	RpcMessage ret = sendMessageSync(rq, rpc_timeout);
	if(!ret.isResponse())
		SHVCHP_EXCEPTION("Invalid response!");
	//logRpc() << "<-- sync method call ret:" << ret.id();
	return RpcResponse(ret);
}

static int s_defaultRpcTimeout = 5000;

int AbstractRpcConnection::defaultRpcTimeout()
{
	return s_defaultRpcTimeout;
}

int AbstractRpcConnection::setDefaultRpcTimeout(int rpc_timeout)
{
	int old = s_defaultRpcTimeout;
	s_defaultRpcTimeout = rpc_timeout;
	return old;
}

} // namespace chainpack
} // namespace shv
