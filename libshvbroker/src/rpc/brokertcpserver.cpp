#include "brokertcpserver.h"
#include "clientconnection.h"

#include <shv/iotqt/rpc/socket.h>

namespace shv {
namespace broker {
namespace rpc {

BrokerTcpServer::BrokerTcpServer(QObject *parent)
	: Super(parent)
{

}

ClientConnection *BrokerTcpServer::connectionById(int connection_id)
{
	return qobject_cast<rpc::ClientConnection *>(Super::connectionById(connection_id));
}

shv::iotqt::rpc::ServerConnection *BrokerTcpServer::createServerConnection(QTcpSocket *socket, QObject *parent)
{
	return new ClientConnection(new shv::iotqt::rpc::TcpSocket(socket), parent);
}

}}}
