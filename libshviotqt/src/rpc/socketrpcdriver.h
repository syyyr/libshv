#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcdriver.h>

#include <QObject>

class QTcpSocket;
class QThread;

namespace shv { namespace chainpack { class RpcRequest; class RpcResponse; }}

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT SocketRpcDriver : public QObject, public shv::chainpack::RpcDriver
{
	Q_OBJECT

	using Super = shv::chainpack::RpcDriver;
public:
	explicit SocketRpcDriver(QObject *parent = nullptr);
	~SocketRpcDriver() Q_DECL_OVERRIDE;

	void setSocket(QTcpSocket *socket);
	void setProtocolVersionAsInt(int v) {Super::setProtocolVersion((shv::chainpack::Rpc::ProtocolVersion)v);}

	void connectToHost(const QString &host_name, quint16 port);

	void sendMessage(const shv::chainpack::RpcValue &msg) {Super::sendMessage(msg);}

	Q_SIGNAL void rpcMessageReceived(shv::chainpack::RpcValue msg);

	// function waits till response is received in event loop
	// rpcMessageReceived signal can be emited meanwhile
	void sendRequestQuasiSync(const shv::chainpack::RpcRequest& request, shv::chainpack::RpcResponse *presponse, int time_out_ms);

	void abortConnection();
protected:
	void setSocketConnected(bool b);
public:
	bool isSocketConnected() const;
	Q_SIGNAL void socketConnectedChanged(bool is_connected);

	int connectionId() const {return m_connectionId;}

	std::string peerAddress() const;
	int peerPort() const;

	int callMethodASync(const std::string &method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	void sendResponse(int request_id, const shv::chainpack::RpcValue &result);
	void sendError(int request_id, const shv::chainpack::RpcResponse::Error &error);
protected:
	// RpcDriver interface
	bool isOpen() Q_DECL_OVERRIDE;
	int64_t writeBytes(const char *bytes, size_t length) Q_DECL_OVERRIDE;
	bool flush() Q_DECL_OVERRIDE;
	void onRpcValueReceived(const shv::chainpack::RpcValue &msg) Q_DECL_OVERRIDE;

	QTcpSocket* socket();
	void onReadyRead();
	void onBytesWritten();
protected:
	QTcpSocket *m_socket = nullptr;
private:
	bool m_isSocketConnected = false;
	int m_connectionId;
};

}}}

