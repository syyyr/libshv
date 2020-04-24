#include "brokertcpserver.h"
#include "serverconnectionbroker.h"

#include <shv/iotqt/rpc/socket.h>

namespace shv {
namespace broker {
namespace rpc {

BrokerTcpServer::BrokerTcpServer(QObject *parent)
	: Super(parent)
{

}

ServerConnectionBroker *BrokerTcpServer::connectionById(int connection_id)
{
	return qobject_cast<rpc::ServerConnectionBroker *>(Super::connectionById(connection_id));
}

shv::iotqt::rpc::ServerConnection *BrokerTcpServer::createServerConnection(QTcpSocket *socket, QObject *parent)
{
	return new ServerConnectionBroker(new shv::iotqt::rpc::TcpSocket(socket), parent);
}

}}}
