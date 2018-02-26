#pragma once

#include "../shviotqtglobal.h"
#include "../chainpack/socketrpcdriver.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/utils.h>

#include <QObject>

#include <string>

class QTcpSocket;
//class Agent;

//namespace shv { namespace chainpack { class RpcMessage; class RpcValue; }}
//namespace shv { namespace coreqt { namespace chainpack { class RpcConnection; }}}

namespace shv {
namespace iotqt {
namespace server {

class SHVIOTQT_DECL_EXPORT ServerConnection : public shv::iotqt::chainpack::SocketRpcDriver
{
	Q_OBJECT

	using Super = shv::iotqt::chainpack::SocketRpcDriver;

	SHV_FIELD_BOOL_IMPL(e, E, choEnabled)

public:
	explicit ServerConnection(QTcpSocket* socket, QObject *parent = 0);
	~ServerConnection() Q_DECL_OVERRIDE;

	const std::string& connectionName() {return m_connectionName;}
	void setConnectionName(const std::string &n) {m_connectionName = n; setObjectName(QString::fromStdString(n));}

protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolVersion protocol_version, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;
	//bool initConnection(const chainpack::RpcValue &rpc_val) Q_DECL_OVERRIDE;
	virtual bool login(const shv::chainpack::RpcValue &auth_params) = 0;
	bool isLoginPhase() const {return !m_loginReceived;}
protected:
	std::string m_connectionName;
	std::string m_user;
	std::string m_pendingAuthNonce;
	bool m_helloReceived = false;
	bool m_loginReceived = false;
};

}}}

