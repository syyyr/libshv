#include "rpcresponsecallback.h"
#include "clientconnection.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/coreqt/log.h>

#include <QTimer>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

RpcResponseCallBack::RpcResponseCallBack(int rq_id, QObject *parent)
	: QObject(parent)
{
	setRequestId(rq_id);
}

RpcResponseCallBack::RpcResponseCallBack(ClientConnection *conn, int rq_id, QObject *parent)
	: RpcResponseCallBack(rq_id, parent)
{
	connect(conn, &ClientConnection::rpcMessageReceived, this, &RpcResponseCallBack::onRpcMessageReceived);
}

void RpcResponseCallBack::start()
{
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
	shvLogFuncFrame() << this << msg.toPrettyString();
	if(!msg.isResponse())
		return;
	cp::RpcResponse rsp(msg);
	if(rsp.peekCallerId() != 0 || !(rsp.requestId() == requestId()))
		return;
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

} // namespace rpc
} // namespace iotqt
} // namespace shv
