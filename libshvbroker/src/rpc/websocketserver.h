#pragma once

#include <QWebSocketServer>

class QWebSocket;
class ServerConnection;

namespace shv {
namespace broker {
namespace rpc {

class BrokerClientServerConnection;

class WebSocketServer : public QWebSocketServer
{
	Q_OBJECT
	using Super = QWebSocketServer;
public:
	WebSocketServer(SslMode secureMode, QObject *parent = nullptr);
	~WebSocketServer() override;

	bool start(int port = 0);

	std::vector<int> connectionIds() const;
	BrokerClientServerConnection* connectionById(int connection_id);
private:
	BrokerClientServerConnection* createServerConnection(QWebSocket *socket, QObject *parent);
	void onNewConnection();
	void unregisterConnection(int connection_id);
private:
	std::map<int, BrokerClientServerConnection*> m_connections;
};

}}}
