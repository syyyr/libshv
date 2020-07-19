#pragma once

#include "socketrpcconnection.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcdriver.h>
#include <shv/chainpack/irpcconnection.h>

#include <shv/core/utils.h>
#include <shv/coreqt/utils.h>

#include <QObject>

class QTimer;

namespace shv {
namespace iotqt {
namespace rpc {

class ClientAppCliOptions;

class SHVIOTQT_DECL_EXPORT ClientConnection : public SocketRpcConnection
{
	Q_OBJECT
	using Super = SocketRpcConnection;

	SHV_FIELD_IMPL(std::string, u, U, ser)
	SHV_FIELD_IMPL(std::string, h, H, ost)
	SHV_FIELD_IMPL2(int, p, P, ort, shv::chainpack::IRpcConnection::DEFAULT_RPC_BROKER_PORT)
	SHV_FIELD_IMPL(std::string, p, P, assword)
	SHV_FIELD_IMPL(shv::chainpack::IRpcConnection::LoginType, l, L, oginType)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, c, C, onnectionOptions)

public:
	enum class State {NotConnected = 0, Connecting, SocketConnected, BrokerConnected, ConnectionError};
	static const char* stateToString(State state);
public:
	explicit ClientConnection(QObject *parent = nullptr);
	~ClientConnection() Q_DECL_OVERRIDE;

	virtual void open();
	void close() override;
	void abort() override;
	void restartIfActive();

	void setCliOptions(const ClientAppCliOptions *cli_opts);

	void setTunnelOptions(const shv::chainpack::RpcValue &opts);

	void setCheckBrokerConnectedInterval(int ms);

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	bool isBrokerConnected() const {return state() == State::BrokerConnected;}
	Q_SIGNAL void brokerConnectedChanged(bool is_connected);
	Q_SIGNAL void brokerLoginError(const std::string &err);

	State state() const { return m_connectionState.state; }
	Q_SIGNAL void stateChanged(State state);

	const shv::chainpack::RpcValue::Map &loginResult() const { return m_connectionState.loginResult.toMap(); }

	//std::string brokerClientPath() const {return brokerClientPath(brokerClientId());}
	//std::string brokerMountPoint() const;
public:
	/// AbstractRpcConnection interface implementation
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
protected:
	void setState(State state);

	void sendHello();
	void sendLogin(const shv::chainpack::RpcValue &server_hello);

	void checkBrokerConnected();
	void whenBrokerConnectedChanged(bool b);
	void emitInitPhaseError(const std::string &err);

	void onSocketConnectedChanged(bool is_connected);
	void onRpcValueReceived(const shv::chainpack::RpcValue &rpc_val) override;

	bool isInitPhase() const {return state() == State::SocketConnected;}
	void processInitPhase(const chainpack::RpcMessage &msg);
	shv::chainpack::RpcValue createLoginParams(const shv::chainpack::RpcValue &server_hello);

	int brokerClientId() const;

	struct ConnectionState
	{
		State state = State::NotConnected;
		int helloRequestId = 0;
		int loginRequestId = 0;
		int pingRqId = 0;
		shv::chainpack::RpcValue loginResult;
	};
	ConnectionState m_connectionState;
private:
	QTimer *m_checkConnectedTimer;
	int m_checkBrokerConnectedInterval = 0;
	QTimer *m_heartBeatTimer = nullptr;
	int m_heartbeatInterval = 0;
};

} // namespace chainpack
} // namespace coreqt
} // namespace shv

