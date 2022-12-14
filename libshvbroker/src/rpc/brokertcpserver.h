#pragma once

#include <shv/iotqt/rpc/tcpserver.h>

class QSslConfiguration;

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
	BrokerTcpServer(SslMode ssl_mode, QObject *parent = nullptr);
	~BrokerTcpServer() override;

	ClientConnectionOnBroker* connectionById(int connection_id);
	bool loadSslConfig();
protected:
	void incomingConnection(qintptr socket_descriptor) override;
	shv::iotqt::rpc::ServerConnection* createServerConnection(QTcpSocket *socket, QObject *parent) override;
protected:
	SslMode m_sslMode;
	// pointer must be used because QSslConfiguration() prints some errors in qt 5.15.2
	//qt.network.ssl: QSslSocket: cannot resolve EVP_PKEY_base_id
	//qt.network.ssl: QSslSocket: cannot resolve SSL_get_peer_certificate
	QSslConfiguration *m_sslConfiguration = nullptr;
};

}}}
