#pragma once

#include <QWebSocketServer>

class QWebSocket;
class ServerConnection;

namespace shv {
namespace broker {
namespace rpc {

class ServerConnectionBroker;

class WebSocketServer : public QWebSocketServer
{
	Q_OBJECT
	using Super = QWebSocketServer;
public:
	WebSocketServer(SslMode secureMode, QObject *parent = nullptr);
	~WebSocketServer() override;

	bool start(int port = 0);

	std::vector<int> connectionIds() const;
	ServerConnectionBroker* connectionById(int connection_id);
private:
	ServerConnectionBroker* createServerConnection(QWebSocket *socket, QObject *parent);
	void onNewConnection();
	void onConnectionClosed(int connection_id);
private:
	std::map<int, ServerConnectionBroker*> m_connections;
};

}}}
