#include "brokertcpserver.h"
#include "clientconnection.h"
#include "../brokerapp.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/socket.h>
#include <QSslSocket>

#include <QFile>
#include <QDir>
#include <QSslKey>

namespace shv {
namespace broker {
namespace rpc {

static QSslConfiguration load_ssl_configuration()
{
	AppCliOptions *opts = BrokerApp::instance()->cliOptions();
	QString cert_fn = QString::fromStdString(opts->serverSslCertFile());
	if(QDir::isRelativePath(cert_fn))
		cert_fn = QString::fromStdString(opts->configDir()) + '/' + cert_fn;
	QString key_fn = QString::fromStdString(opts->serverSslKeyFile());
	if(QDir::isRelativePath(key_fn))
		key_fn = QString::fromStdString(opts->configDir()) + '/' + key_fn;
	QSslConfiguration sslConfiguration;
	QFile certFile(cert_fn);
	QFile keyFile(key_fn);
	certFile.open(QIODevice::ReadOnly);
	keyFile.open(QIODevice::ReadOnly);
	QSslCertificate certificate(&certFile, QSsl::Pem);
	QSslKey sslKey(&keyFile, QSsl::Rsa, QSsl::Pem);
	certFile.close();
	keyFile.close();
	sslConfiguration.setPeerVerifyMode(QSslSocket::VerifyNone);
	sslConfiguration.setLocalCertificate(certificate);
	sslConfiguration.setPrivateKey(sslKey);
	sslConfiguration.setProtocol(QSsl::TlsV1SslV3);
	return sslConfiguration;
}

BrokerTcpServer::BrokerTcpServer(SslMode ssl_mode, QObject *parent)
	: Super(parent), m_sslMode(ssl_mode), m_sslConfiguration(load_ssl_configuration())
{

}

ClientConnection *BrokerTcpServer::connectionById(int connection_id)
{
	return qobject_cast<rpc::ClientConnection *>(Super::connectionById(connection_id));
}

void BrokerTcpServer::incomingConnection(qintptr socket_descriptor)
{
	if (m_sslMode == SecureMode) {
		QSslSocket *sock = new QSslSocket(this);
		if (!sock->setSocketDescriptor(socket_descriptor)) {
			shvError() << "Can't accept connection: setSocketDescriptor error";
			return;
		}
		sock->setSslConfiguration(m_sslConfiguration);
		sock->startServerEncryption();
		addPendingConnection(sock);
	} else if (m_sslMode == NonSecureMode) {
		QTcpSocket *sock = new QTcpSocket(this);
		if (!sock->setSocketDescriptor(socket_descriptor)) {
			shvError() << "Can't accept connection: setSocketDescriptor error";
			return;
		}
		addPendingConnection(sock);
	}
}

shv::iotqt::rpc::ServerConnection *BrokerTcpServer::createServerConnection(QTcpSocket *socket, QObject *parent)
{
	if (m_sslMode == SecureMode)
		return new ClientConnection(new shv::iotqt::rpc::SslSocket(qobject_cast<QSslSocket *>(socket)), parent);
	else
		return new ClientConnection(new shv::iotqt::rpc::TcpSocket(socket), parent);
}

}}}
