#include "socketrpcconnection.h"
#include "rpc.h"

#include <shv/core/exception.h>
#include <shv/core/log.h>

#include <QThread>

#define logRpcMsg() shvCDebug("RpcMsg")
#define logRpcSyncCalls() shvCDebug("RpcSyncCalls")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

SocketRpcConnection::SocketRpcConnection(SyncCalls sync_calls, QObject *parent)
	: QObject(parent)
	, m_syncCalls(sync_calls)
{
	Rpc::registerMetatTypes();

	static int id = 0;
	m_connectionId = ++id;
	m_rpcDriver = new SocketRpcDriver();

	connect(this, &SocketRpcConnection::setProtocolVersionRequest, m_rpcDriver, &SocketRpcDriver::setProtocolVersionAsInt);
	connect(this, &SocketRpcConnection::sendMessageRequest, m_rpcDriver, &SocketRpcDriver::sendMessage);
	connect(this, &SocketRpcConnection::connectToHostRequest, m_rpcDriver, &SocketRpcDriver::connectToHost);
	connect(this, &SocketRpcConnection::abortConnectionRequest, m_rpcDriver, &SocketRpcDriver::abortConnection);

	connect(m_rpcDriver, &SocketRpcDriver::socketConnectedChanged, this, &SocketRpcConnection::socketConnectedChanged);
	connect(m_rpcDriver, &SocketRpcDriver::rpcValueReceived, this, &SocketRpcConnection::onRpcValueReceived);

	if(m_syncCalls == SyncCalls::Supported) {
		connect(this, &SocketRpcConnection::sendMessageSyncRequest, m_rpcDriver, &SocketRpcDriver::sendRequestQuasiSync, Qt::BlockingQueuedConnection);
		m_rpcDriverThread = new QThread();
		m_rpcDriver->moveToThread(m_rpcDriverThread);
		m_rpcDriverThread->start();
	}
	else {
		connect(this, &SocketRpcConnection::sendMessageSyncRequest, []() {
			shvError() << "Sync calls are enabled in threaded RPC connection only!";
		});
	}
}

SocketRpcConnection::~SocketRpcConnection()
{
	shvDebug() << __FUNCTION__;
	if(m_syncCalls == SyncCalls::Supported) {
		if(m_rpcDriverThread->isRunning()) {
			shvDebug() << "stopping rpc driver thread";
			m_rpcDriverThread->quit();
			shvDebug() << "after quit";
		}
		shvDebug() << "joining rpc driver thread";
		bool ok = m_rpcDriverThread->wait();
		shvDebug() << "rpc driver thread joined ok:" << ok;
		delete m_rpcDriverThread;
	}
	delete m_rpcDriver;
}

void SocketRpcConnection::setSocket(QTcpSocket *socket)
{
	m_rpcDriver->setSocket(socket);
}

void SocketRpcConnection::connectToHost(const std::string &host_name, quint16 port)
{
	emit connectToHostRequest(QString::fromStdString(host_name), port);
}

bool SocketRpcConnection::isSocketConnected() const
{
	return m_rpcDriver->isSocketConnected();
}

void SocketRpcConnection::abort()
{
	emit abortConnectionRequest();
}
/*
void SocketRpcConnection::onRpcValueReceived(const SocketRpcConnection::RpcValue &rpc_val)
{
	RpcMessage msg(rpc_val);
	if(!onRpcValueReceived(msg))
		emit messageReceived(msg);
}
*/
bool SocketRpcConnection::onRpcValueReceived(const shv::chainpack::RpcValue &val)
{
	cp::RpcMessage msg(val);
	RpcValue::UInt id = msg.id();
	if(id > 0 && id <= m_maxSyncMessageId) {
		// ignore messages alredy processed by sync calls
		logRpcSyncCalls() << "XXX ignoring already served sync response:" << id;
		return true;
	}
	//logRpcMsg() << cp::RpcDriver::RCV_LOG_ARROW << msg.toStdString();
	return false;
}

void SocketRpcConnection::sendMessage(const SocketRpcConnection::RpcMessage &rpc_msg)
{
	//logRpcMsg() << cp::RpcDriver::SND_LOG_ARROW << rpc_msg.toStdString();
	emit sendMessageRequest(rpc_msg.value());
}

SocketRpcConnection::RpcResponse SocketRpcConnection::sendMessageSync(const SocketRpcConnection::RpcRequest &rpc_request_message, int time_out_ms)
{
	RpcResponse res_msg;
	m_maxSyncMessageId = qMax(m_maxSyncMessageId, rpc_request_message.id());
	//logRpcSyncCalls() << "==> send SYNC MSG id:" << rpc_request_message.id() << "data:" << rpc_request_message.toStdString();
	emit sendMessageSyncRequest(rpc_request_message, &res_msg, time_out_ms);
	//logRpcSyncCalls() << "<== RESP SYNC MSG id:" << res_msg.id() << "data:" << res_msg.toStdString();
	return res_msg;
}

void SocketRpcConnection::sendNotify(const std::string &method, const SocketRpcConnection::RpcValue &params)
{
	RpcRequest rq;
	rq.setMethod(method);
	rq.setParams(params);
	sendMessage(rq);
}

void SocketRpcConnection::sendResponse(int request_id, const SocketRpcConnection::RpcValue &result)
{
	RpcResponse resp;
	resp.setId(request_id);
	resp.setResult(result);
	sendMessage(resp);
}

void SocketRpcConnection::sendError(int request_id, const shv::chainpack::RpcResponse::Error &error)
{
	RpcResponse resp;
	resp.setId(request_id);
	resp.setError(error);
	sendMessage(resp);
}

namespace {
int nextRpcId()
{
	static int n = 0;
	return ++n;
}
}
int SocketRpcConnection::callMethodASync(const std::string &method, const SocketRpcConnection::RpcValue &params)
{
	int id = nextRpcId();
	RpcRequest rq;
	rq.setId(id);
	rq.setMethod(method);
	rq.setParams(params);
	sendMessage(rq);
	return id;
}

SocketRpcConnection::RpcResponse SocketRpcConnection::callMethodSync(const std::string &method, const SocketRpcConnection::RpcValue &params, int rpc_timeout)
{
	return callShvMethodSync(std::string(), method, params, rpc_timeout);
}

SocketRpcConnection::RpcResponse SocketRpcConnection::callShvMethodSync(const std::string &shv_path, const std::string &method, const SocketRpcConnection::RpcValue &params, int rpc_timeout)
{
	RpcRequest rq;
	rq.setId(nextRpcId());
	rq.setMethod(method);
	rq.setParams(params);
	if(!shv_path.empty())
		rq.setShvPath(shv_path);
	//logRpc() << "--> sync method call:" << id << method;
	RpcMessage ret = sendMessageSync(rq, rpc_timeout);
	if(!ret.isResponse())
		SHV_EXCEPTION("Invalid response!");
	//logRpc() << "<-- sync method call ret:" << ret.id();
	return RpcResponse(ret);
}

} // namespace chainpack
} // namespace coreqt
} // namespace shv
