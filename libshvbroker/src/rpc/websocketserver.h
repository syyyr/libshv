#pragma once

#include <QWebSocketServer>

class QWebSocket;
class ServerConnection;

namespace shv {
namespace broker {
namespace rpc {

class ClientConnection;

class WebSocketServer : public QWebSocketServer
{
	Q_OBJECT
	using Super = QWebSocketServer;
public:
	WebSocketServer(SslMode secureMode, QObject *parent = nullptr);
	~WebSocketServer() override;

	bool start(int port = 0);

	std::vector<int> connectionIds() const;
	ClientConnection* connectionById(int connection_id);
private:
	ClientConnection* createServerConnection(QWebSocket *socket, QObject *parent);
	void onNewConnection();
	void unregisterConnection(int connection_id);
private:
	std::map<int, ClientConnection*> m_connections;
};

}}}
