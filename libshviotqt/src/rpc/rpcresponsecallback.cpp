#include "rpcresponsecallback.h"

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

void RpcResponseCallBack::start(RpcResponseCallBack::CallBackFunction cb)
{
	m_callBackFunction = cb;
	if(!m_timeoutTimer) {
		m_timeoutTimer = new QTimer(this);
		m_timeoutTimer->setSingleShot(true);
		connect(m_timeoutTimer, &QTimer::timeout, this, &RpcResponseCallBack::deleteLater);
	}
	m_timeoutTimer->start(timeout());
}

void RpcResponseCallBack::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	shvLogFuncFrame() << this << msg.toPrettyString();
	if(!msg.isResponse())
		return;
	cp::RpcResponse rsp(msg);
	if(rsp.peekCallerId() != 0 || !(rsp.requestId() == requestId()))
		return;
	if(m_callBackFunction)
		m_callBackFunction(rsp);
	deleteLater();
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
