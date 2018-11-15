#pragma once

#include "../shviotqtglobal.h"
#include "socketrpcdriver.h"
#include "iclientconnection.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcdriver.h>
#include <shv/chainpack/abstractrpcconnection.h>

#include <shv/core/utils.h>
#include <shv/coreqt/utils.h>

#include <QObject>

class QTimer;

namespace shv {
namespace iotqt {
namespace rpc {

class ClientAppCliOptions;

/**
  TODO remove duplicate code

  There is lot of code duplicated in ClientConnection and SyncClientConnection, mainly in login phase
  Since SyncClientConnection is not used for now, this duplication is ignored
*/
class SHVIOTQT_DECL_EXPORT SyncClientConnection : public QObject, public IClientConnection, public shv::chainpack::AbstractRpcConnection
{
	Q_OBJECT

public:
	static constexpr int WAIT_FOREVER = -1;
	static constexpr int DEFAULT_RPC_TIMEOUT = 0;

	explicit SyncClientConnection(QObject *parent = nullptr);
	~SyncClientConnection() Q_DECL_OVERRIDE;

	void open();
	void close() Q_DECL_OVERRIDE;
	void abort() Q_DECL_OVERRIDE;
	void resetConnection() override;

	static int defaultRpcTimeout();
	static int setDefaultRpcTimeout(int rpc_timeout);

	void setCliOptions(const ClientAppCliOptions *cli_opts);

	void setCheckBrokerConnectedInterval(unsigned ms);

	void setSocket(QTcpSocket *socket);
	void setProtocolType(shv::chainpack::Rpc::ProtocolType ver) {emit setProtocolTypeRequest((int)ver);}

	Q_SIGNAL void socketConnectedChanged(bool is_connected);
	bool isSocketConnected() const;

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	Q_SIGNAL void brokerConnectedChanged(bool is_connected);
public:
	/// AbstractRpcConnection interface implementation
	/// since RpcDriver is connected to SocketDriver using queued connection. it is safe to call sendMessage from different thread
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	shv::chainpack::RpcResponse sendMessageSync(const shv::chainpack::RpcRequest &rpc_request, int time_out_ms = DEFAULT_RPC_TIMEOUT);
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
protected:
	Q_SIGNAL void setProtocolTypeRequest(int ver);
	Q_SIGNAL void sendMessageRequest(const shv::chainpack::RpcValue& msg);

	Q_SIGNAL void sendMessageSyncRequest(const shv::chainpack::RpcRequest &request, shv::chainpack::RpcResponse *presponse, int time_out_ms);

	// host_name is QString to avoid qRegisterMetatype<std::string>() for queued connection
	Q_SIGNAL void connectToHostRequest(const QString &host_name, quint16 port);
	Q_SIGNAL void closeConnectionRequest();
	Q_SIGNAL void abortConnectionRequest();

	void onSocketConnectedChanged(bool is_connected) override;

	void sendHello() override;
	void sendLogin(const shv::chainpack::RpcValue &server_hello) override;

	void setBrokerConnected(bool b) override;
	void checkBrokerConnected();
	//virtual void onRpcValueReceived(const shv::chainpack::RpcValue &rpc_val);
	void onRpcValueReceived(const shv::chainpack::RpcValue &val);
private:
	SocketRpcDriver *m_rpcDriver = nullptr;
	// RpcDriver must run in separate thread to implement synchronous RPC calls properly
	// QEventLoop solution is not enough, this enable sinc calls within sync call
	QThread *m_rpcDriverThread = nullptr;

	unsigned m_maxSyncMessageId = 0;

	QTimer *m_checkConnectedTimer;
	QTimer *m_pingTimer = nullptr;
};

} // namespace chainpack
} // namespace coreqt
} // namespace shv

