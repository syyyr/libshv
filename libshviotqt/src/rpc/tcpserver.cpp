#include "serverconnection.h"
#include "tcpserver.h"

#include <shv/coreqt/log.h>

#include <QTcpSocket>

namespace shv {
namespace iotqt {
namespace rpc {

TcpServer::TcpServer(QObject *parent)
	: Super(parent)
{
	connect(this, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);
}

TcpServer::~TcpServer()
{
	shvInfo() << "Destroying SHV TcpServer";
}

bool TcpServer::start(int port)
{
	shvInfo() << "Starting RPC server on port:" << port;
	if (!listen(QHostAddress::AnyIPv4, port)) {
		shvError() << tr("Unable to start the server: %1.").arg(errorString());
		close();
		return false;
	}
	shvInfo().nospace() << "RPC server is listenning on " << serverAddress().toString() << ":" << serverPort();
	return true;
}

std::vector<int> TcpServer::connectionIds() const
{
	std::vector<int> ret;
	for(const auto &pair : m_connections)
		ret.push_back(pair.first);
	return ret;
}

ServerConnection *TcpServer::connectionById(int connection_id)
{
	auto it = m_connections.find(connection_id);
	if(it == m_connections.end())
		return nullptr;
	return it->second;
}

void TcpServer::onNewConnection()
{
	QTcpSocket *sock = nextPendingConnection();
	if(sock) {
		shvInfo().nospace() << "client connected: " << sock->peerAddress().toString() << ':' << sock->peerPort();// << "socket:" << sock << sock->socketDescriptor() << "state:" << sock->state();
		ServerConnection *c = createServerConnection(sock, this);
		m_connections[c->connectionId()] = c;
		int cid = c->connectionId();
		connect(c, &ServerConnection::destroyed, [this, cid]() {
			onConnectionDeleted(cid);
		});
	}
}

void TcpServer::onConnectionDeleted(int connection_id)
{
	m_connections.erase(connection_id);
}

}}}
