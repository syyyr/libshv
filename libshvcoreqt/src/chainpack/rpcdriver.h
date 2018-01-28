#pragma once

#include "../shvcoreqtglobal.h"

#include <shv/chainpack/rpcdriver.h>

#include <QObject>

class QTcpSocket;
class QThread;

namespace shv { namespace chainpack { class RpcRequest; class RpcResponse; }}

namespace shv {
namespace coreqt {
namespace chainpack {

class SHVCOREQT_DECL_EXPORT RpcDriver : public QObject, public shv::chainpack::RpcDriver
{
	Q_OBJECT

	using Super = shv::chainpack::RpcDriver;
public:
	explicit RpcDriver(QObject *parent = nullptr);
	~RpcDriver() Q_DECL_OVERRIDE;

	void setSocket(QTcpSocket *socket);
	void setProtocolVersionAsInt(int v) {Super::setProtocolVersion((shv::chainpack::Rpc::ProtocolVersion)v);}

	void connectToHost(const QString &host_name, quint16 port);

	void sendMessage(const shv::chainpack::RpcValue &msg) {Super::sendMessage(msg);}

	Q_SIGNAL void rpcMessageReceived(shv::chainpack::RpcValue msg);

	// function waits till response is received in event loop
	// rpcMessageReceived signal can be emited meanwhile
	void sendRequestQuasiSync(const shv::chainpack::RpcRequest& request, shv::chainpack::RpcResponse *presponse, int time_out_ms);

	void abortConnection();
	bool isConnected() const;
	Q_SIGNAL void connectedChanged(bool is_connected);

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
	bool m_isConnected = false;
};

}}}

