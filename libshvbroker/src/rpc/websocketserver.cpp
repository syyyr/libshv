#include "websocketserver.h"

#include "serverconnectionbroker.h"
#include "websocket.h"
#include "../brokerapp.h"

#include <shv/coreqt/log.h>

#include <QFile>
#include <QDir>
#include <QSslKey>
#include <QWebSocket>

namespace shv {
namespace broker {
namespace rpc {

WebSocketServer::WebSocketServer(SslMode secureMode, QObject *parent)
	: Super("shvbroker", secureMode, parent)
{
	if (secureMode == SecureMode) {
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
		setSslConfiguration(sslConfiguration);
	}
	connect(this, &WebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
}

WebSocketServer::~WebSocketServer()
{
}

bool WebSocketServer::start(int port)
{
	quint16 p = port == 0? 3777: static_cast<quint16>(port);
	shvInfo() << "Starting" << (secureMode() == QWebSocketServer::SecureMode? "secure (wss:)": "not-secure (ws:)") << "WebSocket server on port:" << p;
	if (!listen(QHostAddress::AnyIPv4, p)) {
		shvError() << tr("Unable to start the server: %1.").arg(errorString());
		close();
		return false;
	}
	shvInfo().nospace()
			<< "WebSocket RPC server is listenning on "
			<< serverAddress().toString() << ":" << serverPort()
			<< " in " << (secureMode() == QWebSocketServer::SecureMode? " secure (wss:)": " not-secure (ws:)") << " mode";
	return true;
}

std::vector<int> WebSocketServer::connectionIds() const
{
	std::vector<int> ret;
	for(const auto &pair : m_connections)
		ret.push_back(pair.first);
	return ret;
}

ServerConnectionBroker *WebSocketServer::connectionById(int connection_id)
{
	auto it = m_connections.find(connection_id);
	if(it == m_connections.end())
		return nullptr;
	return it->second;
}

ServerConnectionBroker *WebSocketServer::createServerConnection(QWebSocket *socket, QObject *parent)
{
	return new ServerConnectionBroker(new WebSocket(socket), parent);
}

void WebSocketServer::onNewConnection()
{
	shvInfo() << "New WebSocket connection";
	QWebSocket *sock = nextPendingConnection();
	if(sock) {
		ServerConnectionBroker *c = createServerConnection(sock, this);
		shvInfo().nospace() << "web socket client connected: " << sock->peerAddress().toString() << ':' << sock->peerPort()
							<< " connection ID: " << c->connectionId();
		c->setConnectionName(sock->peerAddress().toString().toStdString() + ':' + std::to_string(sock->peerPort()));
		m_connections[c->connectionId()] = c;
		int cid = c->connectionId();
		connect(c, &ServerConnectionBroker::socketConnectedChanged, [this, cid](bool is_connected) {
			if(!is_connected)
				onConnectionClosed(cid);
		});
	}
}

void WebSocketServer::onConnectionClosed(int connection_id)
{
	m_connections.erase(connection_id);
}

}}}
