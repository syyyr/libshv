#pragma once

#include <shv/iotqt/rpc/tcpserver.h>

#include <QSslConfiguration>

namespace shv {
namespace broker {
namespace rpc {

class ClientConnectionOnBroker;

class BrokerTcpServer : public shv::iotqt::rpc::TcpServer
{
	Q_OBJECT
	using Super = shv::iotqt::rpc::TcpServer;

public:
	enum SslMode { SecureMode = 0, NonSecureMode };
public:
	BrokerTcpServer(SslMode ssl_mode = SecureMode, QObject *parent = nullptr);

	ClientConnectionOnBroker* connectionById(int connection_id);
protected:
	void incomingConnection(qintptr socket_descriptor) override;
	shv::iotqt::rpc::ServerConnection* createServerConnection(QTcpSocket *socket, QObject *parent) override;
protected:
	SslMode m_sslMode;
	QSslConfiguration m_sslConfiguration;
};

}}}
