#include "brokertcpserver.h"
#include "brokerclientserverconnection.h"

#include <shv/iotqt/rpc/socket.h>

namespace shv {
namespace broker {
namespace rpc {

BrokerTcpServer::BrokerTcpServer(QObject *parent)
	: Super(parent)
{

}

BrokerClientServerConnection *BrokerTcpServer::connectionById(int connection_id)
{
	return qobject_cast<rpc::BrokerClientServerConnection *>(Super::connectionById(connection_id));
}

shv::iotqt::rpc::ServerConnection *BrokerTcpServer::createServerConnection(QTcpSocket *socket, QObject *parent)
{
	return new BrokerClientServerConnection(new shv::iotqt::rpc::TcpSocket(socket), parent);
}

}}}
