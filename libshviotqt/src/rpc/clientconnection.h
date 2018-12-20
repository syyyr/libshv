#pragma once

#include "iclientconnection.h"
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

#if 0
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
				OnConnectedCall,
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
#endif

class SHVIOTQT_DECL_EXPORT ClientConnection : public SocketRpcDriver, public IClientConnection, public shv::chainpack::AbstractRpcConnection
{
	Q_OBJECT
	using Super = SocketRpcDriver;
public:
	explicit ClientConnection(QObject *parent = nullptr);
	~ClientConnection() Q_DECL_OVERRIDE;

	void open();
	void close() Q_DECL_OVERRIDE;
	void abort() Q_DECL_OVERRIDE;

	void resetConnection() override;

	void setCliOptions(const ClientAppCliOptions *cli_opts);

	void setTunnelOptions(const shv::chainpack::RpcValue &opts);

	void setCheckBrokerConnectedInterval(unsigned ms);

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	bool isBrokerConnected() const {return m_connectionState.isBrokerConnected;}
	Q_SIGNAL void brokerConnectedChanged(bool is_connected);
	Q_SIGNAL void brokerLoginError(const std::string &err);

	//std::string brokerClientPath() const {return brokerClientPath(brokerClientId());}
	//std::string brokerMountPoint() const;
public:
	/// AbstractRpcConnection interface implementation
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
protected:
	void sendHello() override;
	void sendLogin(const shv::chainpack::RpcValue &server_hello) override;

	void checkBrokerConnected();
	void setBrokerConnected(bool b) override;
	void emitInitPhaseError(const std::string &err) override;

	void onSocketConnectedChanged(bool is_connected) override;
	void onRpcValueReceived(const shv::chainpack::RpcValue &rpc_val) override;
private:
	QTimer *m_checkConnectedTimer;
	QTimer *m_pingTimer = nullptr;
	int m_heartbeatInterval = 0;
};

} // namespace chainpack
} // namespace coreqt
} // namespace shv

