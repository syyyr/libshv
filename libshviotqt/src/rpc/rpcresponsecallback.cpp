#include "rpcresponsecallback.h"
#include "clientconnection.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/coreqt/log.h>

#include <QTimer>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

//===================================================
// RpcCall
//===================================================
RpcResponseCallBack::RpcResponseCallBack(int rq_id, QObject *parent)
	: QObject(parent)
{
	setRequestId(rq_id);
}

RpcResponseCallBack::RpcResponseCallBack(ClientConnection *conn, int rq_id, QObject *parent)
	: RpcResponseCallBack(rq_id, parent)
{
	connect(conn, &ClientConnection::rpcMessageReceived, this, &RpcResponseCallBack::onRpcMessageReceived);
	setTimeout(conn->defaultRpcTimeoutMsec());
}

void RpcResponseCallBack::start()
{
	m_isFinished = false;
	if(!m_timeoutTimer) {
		m_timeoutTimer = new QTimer(this);
		m_timeoutTimer->setSingleShot(true);
		connect(m_timeoutTimer, &QTimer::timeout, this, [this]() {
			shv::chainpack::RpcResponse resp;
			resp.setError(shv::chainpack::RpcResponse::Error::create(shv::chainpack::RpcResponse::Error::MethodCallTimeout, "Shv call timeout after: " + std::to_string(m_timeoutTimer->interval()) + " msec."));
			if(m_callBackFunction)
				m_callBackFunction(resp);
			else
				emit finished(resp);
			deleteLater();
		});
	}
	m_timeoutTimer->start(timeout());
}

void RpcResponseCallBack::start(int time_out)
{
	setTimeout(time_out);
	start();
}

void RpcResponseCallBack::start(RpcResponseCallBack::CallBackFunction cb)
{
	m_callBackFunction = cb;
	start();
}

void RpcResponseCallBack::start(int time_out, RpcResponseCallBack::CallBackFunction cb)
{
	setTimeout(time_out);
	start(cb);
}

void RpcResponseCallBack::start(QObject *context, RpcResponseCallBack::CallBackFunction cb)
{
	start(timeout(), context, cb);
}

void RpcResponseCallBack::start(int time_out_msec, QObject *context, RpcResponseCallBack::CallBackFunction cb)
{
	if(context) {
		connect(context, &QObject::destroyed, this, [this]() {
			m_callBackFunction = nullptr;
			deleteLater();
		});
	}
	start(time_out_msec, cb);
}

void RpcResponseCallBack::abort()
{
	shv::chainpack::RpcResponse resp;
	resp.setError(shv::chainpack::RpcResponse::Error::create(shv::chainpack::RpcResponse::Error::MethodCallCancelled, "Shv call aborted"));

	if(m_callBackFunction)
		m_callBackFunction(resp);
	else
		emit finished(resp);
	deleteLater();
}

void RpcResponseCallBack::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	if(m_isFinished)
		return;
	shvLogFuncFrame() << this << msg.toPrettyString();
	if(!msg.isResponse())
		return;
	cp::RpcResponse rsp(msg);
	if(rsp.peekCallerId() != 0 || !(rsp.requestId() == requestId()))
		return;
	m_isFinished = true;
	if(m_timeoutTimer)
		m_timeoutTimer->stop();
	else
		shvWarning() << "Callback was not started, time-out functionality cannot be provided!";
	if(m_callBackFunction)
		m_callBackFunction(rsp);
	else
		emit finished(rsp);
	deleteLater();
}

//===================================================
// RpcCall
//===================================================
RpcCall::RpcCall(ClientConnection *connection)
	: m_rpcConnection(connection)
{
	connect(this, &RpcCall::result, this, &RpcCall::deleteLater);
	connect(this, &RpcCall::error, this, &RpcCall::deleteLater);
}

RpcCall *RpcCall::createSubscribtionRequest(ClientConnection *connection, const QString &shv_path, const QString &method)
{
	RpcCall *rpc = create(connection);
	rpc->setShvPath(cp::Rpc::DIR_BROKER_APP)
			->setMethod(cp::Rpc::METH_SUBSCRIBE)
			->setParams(cp::RpcValue::Map {
							{cp::Rpc::PAR_PATH, shv_path.toStdString()},
							{cp::Rpc::PAR_METHOD, method.toStdString()},
						});
	return rpc;
}

RpcCall *RpcCall::create(ClientConnection *connection)
{
	return new RpcCall(connection);
}

RpcCall *RpcCall::setShvPath(const std::string &shv_path)
{
	m_shvPath = shv_path;
	return this;
}

RpcCall *RpcCall::setShvPath(const char *shv_path)
{
	return setShvPath(std::string(shv_path));
}

RpcCall *RpcCall::setShvPath(const QString &shv_path)
{
	return setShvPath(shv_path.toStdString());
}

RpcCall *RpcCall::setShvPath(const QStringList &shv_path)
{
	return setShvPath(shv_path.join('/'));
}

RpcCall *RpcCall::setMethod(const std::string &method)
{
	m_method = method;
	return this;
}

RpcCall *RpcCall::setMethod(const char *method)
{
	return setMethod(std::string(method));
}

RpcCall *RpcCall::setMethod(const QString &method)
{
	m_method = method.toStdString();
	return this;
}

RpcCall *RpcCall::setParams(const cp::RpcValue &params)
{
	m_params = params;
	return this;
}

void RpcCall::start()
{
	if(m_rpcConnection.isNull()) {
		emit error("RPC connection is NULL");
		return;
	}
	if(!m_rpcConnection->isBrokerConnected()) {
		emit error("RPC connection is not open");
		return;
	}
	int rqid = m_rpcConnection->nextRequestId();
	RpcResponseCallBack *cb = new RpcResponseCallBack(m_rpcConnection, rqid, this);
	cb->start(this, [this](const cp::RpcResponse &resp) {
		if (resp.isSuccess()) {
			emit result(resp.result());
		}
		else {
			emit error(QString::fromStdString(resp.errorString()));
		}
	});
	m_rpcConnection->callShvMethod(rqid, m_shvPath, m_method, m_params);
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
