#include "abstractrpcconnection.h"
#include "exception.h"

namespace shv {
namespace chainpack {

static std::string to_upper(const std::string &s)
{
	std::string ret(s);
	for (size_t i = 0; i < ret.size(); ++i)
		ret[i] = std::toupper(ret[i]);
	return ret;
}

AbstractRpcConnection::AbstractRpcConnection()
	: m_connectionId(nextConnectionId())
{
}

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

int AbstractRpcConnection::nextRequestId()
{
	static int n = 0;
	return ++n;
}

int AbstractRpcConnection::nextConnectionId()
{
	static int n = 0;
	return ++n;
}

int AbstractRpcConnection::callMethod(const RpcRequest &rq)
{
	RpcRequest _rq(rq);
	int id = rq.requestId().toInt();
	if(id == 0) {
		id = nextRequestId();
		_rq.setRequestId(id);
	}
	sendMessage(rq);
	return id;
}

int AbstractRpcConnection::callMethod(std::string method, const RpcValue &params)
{
	return callShvMethod(std::string(), std::move(method), params);
}

int AbstractRpcConnection::callShvMethod(const std::string &shv_path, std::string method, const RpcValue &params)
{
	int id = nextRequestId();
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
/*
RpcResponse AbstractRpcConnection::callMethodSync(const std::string &method, const RpcValue &params, int rpc_timeout)
{
	return callShvMethodSync(std::string(), method, params, rpc_timeout);
}

RpcResponse AbstractRpcConnection::callShvMethodSync(const std::string &shv_path, const std::string &method, const RpcValue &params, int rpc_timeout)
{
	RpcRequest rq;
	rq.setRequestId(nextRequestId());
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
*/
int AbstractRpcConnection::createSubscription(const std::string &shv_path, std::string method)
{
	return callShvMethod(Rpc::DIR_BROKER_APP
					  , Rpc::METH_SUBSCRIBE
					  , RpcValue::Map{
						  {Rpc::PAR_PATH, shv_path},
						  {Rpc::PAR_METHOD, std::move(method)},
						 });
}

std::string AbstractRpcConnection::loginTypeToString(AbstractRpcConnection::LoginType t)
{
	switch(t) {
	case LoginType::Plain: return "PLAIN";
	case LoginType::Sha1: return "SHA1";
	case LoginType::RsaOaep: return "RSAOAEP";
	default: return "INVALID";
	}
}

AbstractRpcConnection::LoginType AbstractRpcConnection::loginTypeFromString(const std::string &s)
{
	std::string typestr = to_upper(s);
	if(typestr == loginTypeToString(LoginType::Plain))
		return LoginType::Plain;
	if(typestr == loginTypeToString(LoginType::Sha1))
		return LoginType::Sha1;
	if(typestr == loginTypeToString(LoginType::RsaOaep))
		return LoginType::RsaOaep;
	return LoginType::Invalid;
}

} // namespace chainpack
} // namespace shv
