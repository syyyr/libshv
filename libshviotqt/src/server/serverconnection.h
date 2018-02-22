#pragma once

#include "../shviotqtglobal.h"

#include <shv/coreqt/chainpack/rpcdriver.h>

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

class SHVIOTQT_DECL_EXPORT ServerConnection : public shv::coreqt::chainpack::RpcDriver
{
	Q_OBJECT

	using Super = shv::coreqt::chainpack::RpcDriver;

	SHV_FIELD_BOOL_IMPL(e, E, choEnabled)

public:
	explicit ServerConnection(QTcpSocket* socket, QObject *parent = 0);
	~ServerConnection() Q_DECL_OVERRIDE;

	QString agentName() {return objectName();}
	void setAgentName(const QString &n) {setObjectName(n);}

	int connectionId() const {return m_connectionId;}

	int callMethodASync(const std::string &method, const shv::chainpack::RpcValue &params = shv::chainpack::RpcValue());
	void sendResponse(int request_id, const shv::chainpack::RpcValue &result);
	void sendError(int request_id, const shv::chainpack::RpcResponse::Error &error);

	//Q_SIGNAL void knockknocReceived(int connection_id, const shv::chainpack::RpcValue::Map &params);
	//Q_SIGNAL void rpcDataReceived(const shv::chainpack::RpcValue::MetaData &meta, const std::string &data);
	//Q_SIGNAL void rpcMessageReceived(const shv::chainpack::RpcMessage &msg);
protected:
	QString peerAddress() const;
	int peerPort() const;
	//void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
	bool initCommunication(const chainpack::RpcValue &rpc_val) Q_DECL_OVERRIDE;
	virtual bool login(const shv::chainpack::RpcValue &auth_params) = 0;
protected:
	std::string m_user;
	std::string m_pendingAuthNonce;
	bool m_helloReceived = false;
	bool m_loginReceived = false;
	int m_connectionId;
};

}}}

