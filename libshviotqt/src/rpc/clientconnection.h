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

class ClientAppCliOptions;

class SHVIOTQT_DECL_EXPORT ConnectionParams : public shv::chainpack::RpcValue::IMap
{
	using Super = shv::chainpack::RpcValue::IMap;
public:
	class SHVIOTQT_DECL_EXPORT MetaType : public shv::chainpack::meta::MetaType
	{
		using Super = shv::chainpack::meta::MetaType;
	public:
		enum {ID = shv::chainpack::meta::GlobalNS::RegisteredMetaTypes::RpcConnectionParams};
		//struct Tag { enum Enum {RequestId = meta::Tag::USER,
		//						MAX};};
		struct Key { enum Enum {
				Host = 1,
				Port,
				User,
				Password,
				//ParentClientId,
				//CallerClientIds,
				//RequestId,
				//TunName,
				MAX
			};
		};

		MetaType();

		static void registerMetaType();
	};
public:
	ConnectionParams();
	ConnectionParams(const shv::chainpack::RpcValue::IMap &m);
	//TunnelParams& operator=(TunnelParams &&o) {Super::operator =(std::move(o)); return *this;}

	shv::chainpack::RpcValue toRpcValue() const;
	//shv::chainpack::RpcValue callerClientIds() const;
};

class SHVIOTQT_DECL_EXPORT ClientConnection : public QObject, public shv::chainpack::AbstractRpcConnection
{
	Q_OBJECT

	SHV_FIELD_IMPL(std::string, u, U, ser)
	SHV_FIELD_IMPL(std::string, h, H, ost)
	SHV_FIELD_IMPL2(int, p, P, ort, shv::chainpack::AbstractRpcConnection::DEFAULT_RPC_BROKER_PORT)
	SHV_FIELD_IMPL(std::string, p, P, assword)

	SHV_FIELD_IMPL(std::string,c, C, onnectionType) // [device | tunnel | client]
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, c, C, onnectionOptions)

	//SHV_FIELD_IMPL2(int, b, B, rokerClientId, 0)
public:
	enum class SyncCalls {Enabled, Disabled};

	explicit ClientConnection(SyncCalls sync_calls, QObject *parent = nullptr);
	explicit ClientConnection(QObject *parent = nullptr) : ClientConnection(SyncCalls::Disabled, parent) {}
	~ClientConnection() Q_DECL_OVERRIDE;

	void open();
	void close() Q_DECL_OVERRIDE;
	void abort() Q_DECL_OVERRIDE;
	void resetConnection();

	void setCliOptions(const ClientAppCliOptions *cli_opts);

	void setCheckBrokerConnectedInterval(unsigned ms);

	void setSocket(QTcpSocket *socket);
	void setProtocolType(shv::chainpack::Rpc::ProtocolType ver) {emit setProtocolTypeRequest((unsigned)ver);}

	int connectionId() const {return m_connectionId;}

	Q_SIGNAL void socketConnectedChanged(bool is_connected);
	bool isSocketConnected() const;

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	bool isBrokerConnected() const {return m_connectionState.isBrokerConnected;}
	Q_SIGNAL void brokerConnectedChanged(bool is_connected);
	const shv::chainpack::RpcValue::Map& loginResult() const {return m_connectionState.loginResult.toMap();}
	unsigned brokerClientId() const;
	static std::string brokerClientPath(unsigned client_id);
	std::string brokerClientPath() const {return brokerClientPath(brokerClientId());}
	std::string brokerMountPoint() const;
public:
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
	void setBrokerConnected(bool b);
private:
	SocketRpcDriver *m_rpcDriver = nullptr;
	// RpcDriver must run in separate thread to implement synchronous RPC calls properly
	// QEventLoop solution is not enough, this enable sinc calls within sync call
	QThread *m_rpcDriverThread = nullptr;
	int m_connectionId;
	SyncCalls m_syncCalls;

	QTimer *m_checkConnectedTimer;
	QTimer *m_pingTimer = nullptr;

	struct ConnectionState
	{
		unsigned maxSyncMessageId = 0;
		bool isBrokerConnected = false;
		unsigned helloRequestId = 0;
		unsigned loginRequestId = 0;
		unsigned pingRqId = 0;
		shv::chainpack::RpcValue loginResult;
	};

	ConnectionState m_connectionState;
	int m_checkBrokerConnectedInterval = 0;
};

} // namespace chainpack
} // namespace coreqt
} // namespace shv

