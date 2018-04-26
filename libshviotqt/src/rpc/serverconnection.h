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

	//SHV_FIELD_BOOL_IMPL(e, E, choEnabled)

public:
	explicit ServerConnection(QTcpSocket* socket, QObject *parent = 0);
	~ServerConnection() Q_DECL_OVERRIDE;

	const std::string& connectionName() {return m_connectionName;}
	void setConnectionName(const std::string &n) {m_connectionName = n; setObjectName(QString::fromStdString(n));}

	void close() Q_DECL_OVERRIDE {closeConnection();}
	void abort() Q_DECL_OVERRIDE {abortConnection();}

	bool isBrokerConnected() const {return isSocketConnected() && !isInitPhase();}

	Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);

	/// AbstractRpcConnection interface implementation
	void sendMessage(const shv::chainpack::RpcMessage &rpc_msg) override;
	shv::chainpack::RpcResponse sendMessageSync(const shv::chainpack::RpcRequest &rpc_request, int time_out_ms = DEFAULT_RPC_TIMEOUT) override;
	void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_version, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
	void onRpcValueReceived(const shv::chainpack::RpcValue &msg) override;

	bool isInitPhase() const {return !m_loginReceived;}
	//bool isInitPhase() const {return !m_loginReceived && (m_sessionClientId == 0 || m_sessionValidated);}
	virtual void processInitPhase(const chainpack::RpcMessage &msg);
	virtual shv::chainpack::RpcValue login(const shv::chainpack::RpcValue &auth_params) = 0;
	//virtual shv::chainpack::RpcValue createLoginResult() = 0;
protected:
	std::string m_connectionName;
	std::string m_user;
	std::string m_pendingAuthNonce;
	bool m_helloReceived = false;
	bool m_loginReceived = false;
	//int m_sessionClientId = 0;
	//bool m_sessionValidated = false;
};

}}}

