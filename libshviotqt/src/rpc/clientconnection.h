#pragma once

#include "../shviotqtglobal.h"
#include "socketrpcdriver.h"

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

class SHVIOTQT_DECL_EXPORT ClientConnection : public QObject, public shv::chainpack::AbstractRpcConnection
{
	Q_OBJECT

	SHV_FIELD_IMPL(std::string, u, U, ser)
	SHV_FIELD_IMPL(std::string, h, H, ost)
	SHV_FIELD_IMPL2(int, p, P, ort, shv::chainpack::AbstractRpcConnection::DEFAULT_RPC_BROKER_PORT)
	SHV_FIELD_IMPL(std::string, p, P, assword)

	SHV_FIELD_IMPL(std::string,c, C, onnectionType) // [device | tunnel | client]
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, c, c, onnectionOptions)

	SHV_PROPERTY_BOOL_IMPL(b, B, rokerConnected)
public:
	enum class SyncCalls {Enabled, Disabled};

	explicit ClientConnection(SyncCalls sync_calls, QObject *parent = nullptr);
	explicit ClientConnection(QObject *parent = nullptr) : ClientConnection(SyncCalls::Enabled, parent) {}
	~ClientConnection() Q_DECL_OVERRIDE;

	void open();
	void close() Q_DECL_OVERRIDE;
	void abort() Q_DECL_OVERRIDE;

	//unsigned checkBrokerConnectedInterval() const {return m_checkBrokerConnectedInterval;}
	void setCheckBrokerConnectedInterval(unsigned ms);

	void setSocket(QTcpSocket *socket);
	void setProtocolType(shv::chainpack::Rpc::ProtocolType ver) {emit setProtocolTypeRequest((unsigned)ver);}

	int connectionId() const {return m_connectionId;}

	Q_SIGNAL void socketConnectedChanged(bool is_connected);
	bool isSocketConnected() const;

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	/// AbstractRpcConnection interface implementation
	/// since RpcDriver is connected to SocketDriver using queued connection. it is safe to call sendMessage from different thread
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	shv::chainpack::RpcResponse sendMessageSync(const shv::chainpack::RpcRequest &rpc_request, int time_out_ms = DEFAULT_RPC_TIMEOUT) override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
protected:
	Q_SIGNAL void setProtocolTypeRequest(int ver);
	Q_SIGNAL void sendMessageRequest(const shv::chainpack::RpcValue& msg);

	Q_SIGNAL void sendMessageSyncRequest(const shv::chainpack::RpcRequest &request, shv::chainpack::RpcResponse *presponse, int time_out_ms);

	// host_name is QString to avoid qRegisterMetatype<std::string>() for queued connection
	Q_SIGNAL void connectToHostRequest(const QString &host_name, quint16 port);
	Q_SIGNAL void closeConnectionRequest();
	Q_SIGNAL void abortConnectionRequest();

	//virtual void onRpcValueReceived(const shv::chainpack::RpcValue &rpc_val);
	void onRpcValueReceived(const shv::chainpack::RpcValue &val);
protected:
	bool isInitPhase() const {return !isBrokerConnected();}
	virtual void processInitPhase(const chainpack::RpcMessage &msg);
	virtual shv::chainpack::RpcValue createLoginParams(const shv::chainpack::RpcValue &server_hello);

	virtual std::string passwordHash(const std::string &user);
	virtual void onSocketConnectedChanged(bool is_connected);
	void sendHello();
	void sendLogin(const shv::chainpack::RpcValue &server_hello);

	void checkBrokerConnected();
	void resetConnection();
private:
	SocketRpcDriver *m_rpcDriver = nullptr;
	// RpcDriver must run in separate thread to implement synchronous RPC calls properly
	// QEventLoop solution is not enough, this enable sinc calls within sync call
	QThread *m_rpcDriverThread = nullptr;
	int m_connectionId;
	SyncCalls m_syncCalls;
	shv::chainpack::RpcValue::UInt m_maxSyncMessageId = 0;

	QTimer *m_checkConnectedTimer;
private:
	unsigned m_helloRequestId = 0;
	unsigned m_loginRequestId = 0;
};

} // namespace chainpack
} // namespace coreqt
} // namespace shv

