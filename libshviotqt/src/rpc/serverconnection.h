#pragma once

#include "../shviotqtglobal.h"
#include "socketrpcdriver.h"

#include <shv/chainpack/abstractrpcconnection.h>
#include <shv/chainpack/rpcmessage.h>

#include <shv/core/utils.h>

#include <QObject>

#include <string>

class QTcpSocket;

//namespace shv { namespace chainpack { class RpcMessage; class RpcValue; }}
//namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT ServerConnection : public SocketRpcDriver, public shv::chainpack::AbstractRpcConnection
{
	Q_OBJECT

	using Super = SocketRpcDriver;
public:
	//enum class ConnectionType {Unknown, Client, Device, Broker};

	explicit ServerConnection(QTcpSocket* socket, QObject *parent = nullptr);
	~ServerConnection() Q_DECL_OVERRIDE;

	//ConnectionType connectionType() const {return m_connectionType;}

	const std::string& connectionName() {return m_connectionName;}
	void setConnectionName(const std::string &n) {m_connectionName = n; setObjectName(QString::fromStdString(n));}

	void close() Q_DECL_OVERRIDE {closeConnection();}
	void abort() Q_DECL_OVERRIDE {abortConnection();}

	const shv::chainpack::RpcValue::Map& connectionOptions() const {return m_connectionOptions.toMap();}

	const std::string& userName() const { return m_userName; }

	virtual bool isConnectedAndLoggedIn() const {return isSocketConnected() && !isInitPhase();}
	virtual bool isSlaveBrokerConnection() const;

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	/// AbstractRpcConnection interface implementation
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	//shv::chainpack::RpcResponse sendMessageSync(const shv::chainpack::RpcRequest &rpc_request, int time_out_ms = DEFAULT_RPC_TIMEOUT) override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
protected:
	enum class PasswordFormat {Invalid, Plain, Sha1};
	static std::string passwordFormatToString(PasswordFormat f);
	static PasswordFormat passwordFormatFromString(const std::string &s);
protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
	void onRpcValueReceived(const shv::chainpack::RpcValue &msg) override;

	bool isInitPhase() const {return !m_loginReceived;}
	//bool isInitPhase() const {return !m_loginReceived && (m_sessionClientId == 0 || m_sessionValidated);}
	virtual void processInitPhase(const chainpack::RpcMessage &msg);
	virtual shv::chainpack::RpcValue login(const shv::chainpack::RpcValue &auth_params);
	virtual bool checkPassword(const shv::chainpack::RpcValue::Map &login);
	virtual std::string passwordHash(LoginType login_type, const std::string &user) = 0;
protected:
	std::string m_connectionName;
	std::string m_userName;
	std::string m_pendingAuthNonce;
	bool m_helloReceived = false;
	bool m_loginReceived = false;
	//ConnectionType m_connectionType = ConnectionType::Unknown;

	//std::string m_connectionType;
	shv::chainpack::RpcValue m_connectionOptions;
};

}}}

