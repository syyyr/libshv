#include "mockrpcconnection.h"

#include <shv/iotqt/rpc/socket.h>

MockRpcConnection::MockRpcConnection(QObject *parent)
	: shv::iotqt::rpc::SocketRpcConnection{parent}
{
	setProtocolType(shv::chainpack::Rpc::ProtocolType::ChainPack);
}

void MockRpcConnection::close()
{
	socket()->close();
}

void MockRpcConnection::abort()
{
	socket()->abort();
}

void MockRpcConnection::sendMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	sendRpcValue(rpc_msg.value());
}

void MockRpcConnection::onRpcMessageReceived(const shv::chainpack::RpcMessage &msg)
{
	emit rpcMessageReceived(msg);
}

