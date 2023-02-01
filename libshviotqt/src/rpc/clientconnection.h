#pragma once

#include "socketrpcconnection.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcdriver.h>
#include <shv/chainpack/irpcconnection.h>

#include <shv/core/utils.h>
#include <shv/coreqt/utils.h>

#include <QObject>
#include <QUrl>

class QTimer;

namespace shv {
namespace iotqt {
namespace rpc {

class ClientAppCliOptions;

class SHVIOTQT_DECL_EXPORT ClientConnection : public SocketRpcConnection
{
	Q_OBJECT
	using Super = SocketRpcConnection;

public:
	enum class State {NotConnected = 0, Connecting, SocketConnected, BrokerConnected, ConnectionError};

	SHV_FIELD_IMPL(std::string, u, U, ser)
	//SHV_FIELD_IMPL(std::string, h, H, ost)
	SHV_FIELD_BOOL_IMPL2(p, P, eerVerify, true)
	SHV_FIELD_IMPL(std::string, p, P, assword)
	SHV_FIELD_IMPL(shv::chainpack::IRpcConnection::LoginType, l, L, oginType)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, c, C, onnectionOptions)
	SHV_FIELD_IMPL2(int, h, H, eartBeatInterval, 60)

public:
	explicit ClientConnection(QObject *parent = nullptr);
	~ClientConnection() Q_DECL_OVERRIDE;

	QUrl connectionUrl() const { return m_connectionUrl; }
	void setConnectionUrl(const QUrl &url);
	void setConnectionString(const QString &connection_string);
	void setHost(const std::string &host) { setConnectionString(QString::fromStdString(host)); }

	static const char* stateToString(State state);

	virtual void open();
	void close() override { closeOrAbort(false); }
	void abort() override { closeOrAbort(true); }

	void setCliOptions(const ClientAppCliOptions *cli_opts);

	void setTunnelOptions(const shv::chainpack::RpcValue &opts);

	void setCheckBrokerConnectedInterval(int ms);
	int checkBrokerConnectedInterval() const { return m_checkBrokerConnectedInterval; }

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	bool isBrokerConnected() const {return state() == State::BrokerConnected;}
	Q_SIGNAL void brokerConnectedChanged(bool is_connected);
	Q_SIGNAL void brokerLoginError(const QString &err);

	State state() const { return m_connectionState.state; }
	Q_SIGNAL void stateChanged(State state);

	const shv::chainpack::RpcValue::Map &loginResult() const { return m_connectionState.loginResult.asMap(); }

	int brokerClientId() const;
	void muteShvPathInLog(std::string shv_path);
public:
	/// AbstractRpcConnection interface implementation
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
	static QUrl connectionUrlFromString(const QString &url_str);
protected:
	void setState(State state);
	void closeOrAbort(bool is_abort);

	void sendHello();
	void sendLogin(const shv::chainpack::RpcValue &server_hello);

	void checkBrokerConnected();
	void whenBrokerConnectedChanged(bool b);
	void emitInitPhaseError(const std::string &err);

	void onSocketConnectedChanged(bool is_connected);
	void onRpcValueReceived(const shv::chainpack::RpcValue &rpc_val) override;

	bool isLoginPhase() const {return state() == State::SocketConnected;}
	void processLoginPhase(const chainpack::RpcMessage &msg);
	shv::chainpack::RpcValue createLoginParams(const shv::chainpack::RpcValue &server_hello) const;

	struct ConnectionState
	{
		State state = State::NotConnected;
		int helloRequestId = 0;
		int loginRequestId = 0;
		int pingRqId = 0;
		shv::chainpack::RpcValue loginResult;
	};
	ConnectionState m_connectionState;

	bool isShvPathMutedInLog(const std::string &shv_path) const;
private:
	bool isAutoConnect() const { return m_checkBrokerConnectedInterval > 0; }
	void restartIfAutoConnect();

	static void tst_connectionUrlFromString();
private:
	QUrl m_connectionUrl;
	QTimer *m_checkBrokerConnectedTimer;
	int m_checkBrokerConnectedInterval = 0;
	QTimer *m_heartBeatTimer = nullptr;
	std::vector<std::string> m_mutedShvPathsInLog;
};

} // namespace chainpack
} // namespace coreqt
} // namespace shv

