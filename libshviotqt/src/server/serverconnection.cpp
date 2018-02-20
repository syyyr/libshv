#include "serverconnection.h"
//#include "../brokerapp.h"

#include <shv/coreqt/chainpack/rpcconnection.h>
#include <shv/coreqt/log.h>

#include <shv/core/shvexception.h>

//#include <shv/chainpack/chainpackprotocol.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;
namespace cpq = shv::coreqt::chainpack;

namespace shv {
namespace iotqt {
namespace server {

static int s_connectionId = 0;

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(parent)
	//, m_socket(socket)
	, m_connectionId(++s_connectionId)
{
	setSocket(socket);
	socket->setParent(nullptr);
	connect(this, &ServerConnection::socketConnectedChanged, [this](bool is_connected) {
		if(!is_connected) {
			this->deleteLater();
		}
		else {
			setAgentName(QStringLiteral("%1:%2").arg(peerAddress()).arg(peerPort()));
		}
	});
}

ServerConnection::~ServerConnection()
{
	shvInfo() << "Agent disconnected:" << agentName();
}

namespace {
int nextRpcId()
{
	static int n = 0;
	return ++n;
}
}
int ServerConnection::callMethodASync(const std::string & method, const cp::RpcValue &params)
{
	int id = nextRpcId();
	cp::RpcRequest rq;
	rq.setRequestId(id);
	rq.setMethod(method);
	rq.setParams(params);
	sendMessage(rq.value());
	return id;
}

void ServerConnection::sendResponse(int request_id, const cp::RpcValue &result)
{
	cp::RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setResult(result);
	sendMessage(resp.value());
}

void ServerConnection::sendError(int request_id, const cp::RpcResponse::Error &error)
{
	cp::RpcResponse resp;
	resp.setRequestId(request_id);
	resp.setError(error);
	sendMessage(resp.value());
}

QString ServerConnection::peerAddress() const
{
	if(m_socket)
		return m_socket->peerAddress().toString();
	return QString();
}

int ServerConnection::peerPort() const
{
	if(m_socket)
		return m_socket->peerPort();
	return -1;
}

}}}


