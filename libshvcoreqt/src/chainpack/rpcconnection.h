#pragma once

#include "../shvcoreqtglobal.h"
#include "rpcdriver.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcdriver.h>

#include <QObject>

//class QHostAddress;
//namespace shv { namespace core { namespace chainpack { class RpcValue; class RpcMessage; class RpcRequest; class RpcResponse; }}}

namespace shv {
namespace coreqt {
namespace chainpack {

class SHVCOREQT_DECL_EXPORT RpcConnection : public QObject
{
	Q_OBJECT
public:
	using RpcValue = shv::chainpack::RpcValue;
	using RpcMessage = shv::chainpack::RpcMessage;
	using RpcRequest = shv::chainpack::RpcRequest;
	using RpcResponse = shv::chainpack::RpcResponse;
	//using RpcDriver = shv::chainpack::RpcDriver;
public:
	enum class SyncCalls {Supported, NotSupported};

	explicit RpcConnection(SyncCalls sync_calls, QObject *parent = nullptr);
	~RpcConnection() Q_DECL_OVERRIDE;

	//static int nextRpcId();
	void setSocket(QTcpSocket *socket);
	void setProtocolVersion(shv::chainpack::Rpc::ProtocolVersion ver) {emit setProtocolVersionRequest((unsigned)ver);}

	int connectionId() const {return m_connectionId;}

	void connectToHost(const QString &host_name, quint16 port);
	Q_SIGNAL void connectedChanged(bool is_connected);
	bool isConnected() const;
	void abort();
	//QHostAddress peerAddress();
	//int peerPort();

	/// since RpcDriver is connected to SocketDriver using queued connection. it is safe to call sendMessage from different thread
	Q_SLOT void sendMessage(const RpcMessage &rpc_msg);
	Q_SLOT void sendNotify(const QString &method, const RpcValue &params = RpcValue());
	Q_SLOT void sendResponse(int request_id, const RpcValue &result);
	Q_SLOT void sendError(int request_id, const RpcResponse::Error &error);
	Q_SLOT int callMethodASync(const QString &method, const RpcValue &params = RpcValue());
	RpcResponse callMethodSync(const QString &method, const RpcValue &params = RpcValue(), int rpc_timeout = 0);
	RpcResponse callShvMethodSync(const QString &shv_path, const QString &method, const RpcValue &params = RpcValue(), int rpc_timeout = 0);

	Q_SIGNAL void messageReceived(const RpcMessage &msg);
	//Q_SIGNAL void rpcError(const QString &err_msg);
	Q_SIGNAL void openChanged(bool is_open);
protected:
	Q_SIGNAL void setProtocolVersionRequest(int ver);
	Q_SIGNAL void sendMessageRequest(const RpcValue& msg);

	Q_SIGNAL void sendMessageSyncRequest(const shv::chainpack::RpcRequest &request, shv::chainpack::RpcResponse *presponse, int time_out_ms);
	Q_SLOT RpcResponse sendMessageSync(const RpcRequest &rpc_request_message, int time_out_ms = 0);

	Q_SIGNAL void connectToHostRequest(const QString &host_name, quint16 port);
	Q_SIGNAL void abortConnectionRequest();

	void onSocketConnected();
	void onSocketDisconnected();

	void onMessageReceived(const RpcValue &rpc_val);
	//virtual void processMessage(const RpcMessage &rpc_msg);
private:
	RpcDriver *m_rpcDriver = nullptr;
	// RpcDriver must run in separate thread to implement synchronous RPC calls properly
	// QEventLoop solution is not enough, this enable sinc calls within sync call
	QThread *m_rpcDriverThread = nullptr;
	int m_connectionId;
	SyncCalls m_syncCalls;
	RpcValue::UInt m_maxSyncMessageId = 0;
};

} // namespace chainpack
} // namespace coreqt
} // namespace shv
