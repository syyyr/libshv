#include "irpcconnection.h"
#include "accessgrant.h"
#include "exception.h"

#include <necrolog.h>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)

namespace shv {
namespace chainpack {

static std::string to_upper(const std::string &s)
{
	std::string ret(s);
	for (size_t i = 0; i < ret.size(); ++i)
		ret[i] = std::toupper(ret[i]);
	return ret;
}

IRpcConnection::IRpcConnection()
	: m_connectionId(nextConnectionId())
{
}

IRpcConnection::~IRpcConnection()
{
}

void IRpcConnection::sendSignal(std::string method, const RpcValue &params)
{
	sendShvNotify(std::string(), std::move(method), params);
}

void IRpcConnection::sendShvSignal(const std::string &shv_path, std::string method, const RpcValue &params)
{
	RpcRequest rq;
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	rq.setMethod(std::move(method));
	rq.setParams(params);
	sendMessage(rq);
}

void IRpcConnection::sendResponse(const RpcValue &request_id, const RpcValue &result)
{
	RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setResult(result);
	sendMessage(resp);
}

void IRpcConnection::sendError(const RpcValue &request_id, const RpcResponse::Error &error)
{
	RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setError(error);
	sendMessage(resp);
}

int IRpcConnection::nextRequestId()
{
	static int n = 0;
	return ++n;
}

int IRpcConnection::nextConnectionId()
{
	static int n = 0;
	return ++n;
}

int IRpcConnection::callMethod(const RpcRequest &rq)
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

int IRpcConnection::callMethod(std::string method, const RpcValue &params)
{
	return callShvMethod(std::string(), std::move(method), params);
}

int IRpcConnection::callShvMethod(const std::string &shv_path, std::string method, const RpcValue &params)
{
	return callShvMethod(shv_path, method, params, AccessGrant());
}

int IRpcConnection::callShvMethod(const std::string &shv_path, std::string method, const RpcValue &params, const AccessGrant &grant)
{
	int id = nextRequestId();
	RpcRequest rq;
	rq.setRequestId(id);
	rq.setMethod(std::move(method));
	if(params.isValid())
		rq.setParams(params);
	if(grant.isValid())
		rq.setAccessGrant(grant);
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	sendMessage(rq);
	return id;
}

int IRpcConnection::callMethodSubscribe(const std::string &shv_path, std::string method, const RpcValue &grant)
{
	logSubscriptionsD() << "call subscribe for connection id:" << connectionId() << "path:" << shv_path << "method:" << method << "grant:" << grant.toCpon();
	return callShvMethod(Rpc::DIR_BROKER_APP
					  , Rpc::METH_SUBSCRIBE
					  , RpcValue::Map{
						  {Rpc::PAR_PATH, shv_path},
						  {Rpc::PAR_METHOD, std::move(method)},
						 }
					  , grant);
}

int IRpcConnection::callMethodUnsubscribe(const std::string &shv_path, std::string method, const RpcValue &grant)
{
	logSubscriptionsD() << "call unsubscribe for connection id:" << connectionId() << "path:" << shv_path << "method:" << method << "grant:" << grant.toCpon();
	return callShvMethod(Rpc::DIR_BROKER_APP
					  , Rpc::METH_UNSUBSCRIBE
					  , RpcValue::Map{
						  {Rpc::PAR_PATH, shv_path},
						  {Rpc::PAR_METHOD, std::move(method)},
						 }
						 , grant);
}

std::string IRpcConnection::loginTypeToString(IRpcConnection::LoginType t)
{
	switch(t) {
	case LoginType::Plain: return "PLAIN";
	case LoginType::Sha1: return "SHA1";
	case LoginType::RsaOaep: return "RSAOAEP";
	default: return "INVALID";
	}
}

IRpcConnection::LoginType IRpcConnection::loginTypeFromString(const std::string &s)
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
