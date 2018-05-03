#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcdriver.h>

#include <QObject>

class QTcpSocket;
class QThread;

//namespace shv { namespace chainpack { class RpcRequest; class RpcResponse; }}

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
	bool hasSocket() const {return m_socket != nullptr;}
	void setProtocolTypeAsInt(int v) {Super::setProtocolType((shv::chainpack::Rpc::ProtocolType)v);}

	void connectToHost(const QString &host_name, quint16 port);

	Q_SIGNAL void rpcValueReceived(shv::chainpack::RpcValue rpc_val);
	Q_SLOT void sendRpcValue(const shv::chainpack::RpcValue &rpc_val) {Super::sendRpcValue(rpc_val);}

	void closeConnection();
	void abortConnection();

	bool isSocketConnected() const;
	Q_SIGNAL void socketConnectedChanged(bool is_connected);

	int connectionId() const {return m_connectionId;}

	std::string peerAddress() const;
	int peerPort() const;
public:
	Q_SLOT void sendRpcRequestSync_helper(const shv::chainpack::RpcRequest& request, shv::chainpack::RpcResponse *presponse, int time_out_ms);
protected:
	// RpcDriver interface
	bool isOpen() Q_DECL_OVERRIDE;
	int64_t writeBytes(const char *bytes, size_t length) Q_DECL_OVERRIDE;
	bool flush() Q_DECL_OVERRIDE;

	QTcpSocket* socket();
	void onReadyRead();
	void onBytesWritten();

	void onRpcValueReceived(const shv::chainpack::RpcValue &rpc_val) override;
	void onProcessReadDataException(std::exception &e) override {Q_UNUSED(e) abortConnection();}
protected:
	QTcpSocket *m_socket = nullptr;
private:
	int m_connectionId;
};

}}}

