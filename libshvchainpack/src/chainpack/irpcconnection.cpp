#include "irpcconnection.h"
#include "accessgrant.h"
#include "exception.h"

#include <necrolog.h>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)

namespace shv {
namespace chainpack {

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
	return callShvMethod(shv_path, method, params, RpcValue());
}

int IRpcConnection::callShvMethod(const std::string &shv_path, std::string method, const RpcValue &params, const RpcValue &grant)
{
	int id = nextRequestId();
	return callShvMethod(id, shv_path, method, params, grant);
}

int IRpcConnection::callShvMethod(int rq_id, const std::string &shv_path, std::string method, const RpcValue &params)
{
	return callShvMethod(rq_id, shv_path, method, params, RpcValue());
}

int IRpcConnection::callShvMethod(int rq_id, const std::string &shv_path, std::string method, const RpcValue &params, const RpcValue &grant)
{
	RpcRequest rq;
	rq.setRequestId(rq_id);
	rq.setMethod(std::move(method));
	if(params.isValid())
		rq.setParams(params);
	if(grant.isValid())
		rq.setAccessGrant(grant);
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	sendMessage(rq);
	return rq_id;
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

} // namespace chainpack
} // namespace shv
