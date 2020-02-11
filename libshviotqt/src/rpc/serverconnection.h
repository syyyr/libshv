#pragma once

#include "../shviotqtglobal.h"
#include "socketrpcconnection.h"

#include <shv/chainpack/irpcconnection.h>
#include <shv/chainpack/rpcmessage.h>

#include <shv/core/utils.h>

#include <QObject>

#include <string>

//namespace shv { namespace chainpack { class RpcMessage; class RpcValue; }}
//namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace shv {
namespace iotqt {
namespace rpc {

class Socket;

class SHVIOTQT_DECL_EXPORT ServerConnection : public SocketRpcConnection
{
	Q_OBJECT

	using Super = SocketRpcConnection;
public:
	explicit ServerConnection(Socket *socket, QObject *parent = nullptr);
	~ServerConnection() Q_DECL_OVERRIDE;

	const std::string& connectionName() {return m_connectionName;}
	void setConnectionName(const std::string &n) {m_connectionName = n; setObjectName(QString::fromStdString(n));}

	void close() Q_DECL_OVERRIDE {closeConnection();}
	void abort() Q_DECL_OVERRIDE {abortConnection();}

	const shv::chainpack::RpcValue::Map& connectionOptions() const {return m_connectionOptions.toMap();}

	const std::string& userName() const { return m_userLogin.user; }

	virtual bool isConnectedAndLoggedIn() const {return isSocketConnected() && !isInitPhase();}

	virtual bool isSlaveBrokerConnection() const;

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	/// AbstractRpcConnection interface implementation
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
	void onRpcValueReceived(const shv::chainpack::RpcValue &msg) override;

	bool isInitPhase() const {return !m_loginReceived;}
	virtual void processInitPhase(const chainpack::RpcMessage &msg);

	virtual void processLoginPhase() = 0;
	virtual void setLoginResult(const shv::chainpack::UserLoginResult &result);
protected:
	std::string m_connectionName;
	shv::chainpack::UserLogin m_userLogin;
	shv::chainpack::UserLoginContext m_userLoginContext;
	bool m_helloReceived = false;
	bool m_loginReceived = false;

	shv::chainpack::RpcValue m_connectionOptions;
};

}}}

