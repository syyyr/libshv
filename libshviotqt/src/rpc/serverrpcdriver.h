#pragma once

#include "../shviotqtglobal.h"
#include "socketrpcdriver.h"

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
namespace rpc {

class SHVIOTQT_DECL_EXPORT ServerRpcDriver : public SocketRpcDriver
{
	Q_OBJECT

	using Super = SocketRpcDriver;

	SHV_FIELD_BOOL_IMPL(e, E, choEnabled)

public:
    explicit ServerRpcDriver(QTcpSocket* socket, QObject *parent = 0);
    ~ServerRpcDriver() Q_DECL_OVERRIDE;

	const std::string& connectionName() {return m_connectionName;}
	void setConnectionName(const std::string &n) {m_connectionName = n; setObjectName(QString::fromStdString(n));}

protected:
	void onRpcDataReceived(shv::chainpack::Rpc::ProtocolVersion protocol_version, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len) override;

    bool isInitPhase() const {return !m_loginReceived;}
    virtual void processInitPhase(const chainpack::RpcMessage &msg);
	virtual bool login(const shv::chainpack::RpcValue &auth_params) = 0;
protected:
	std::string m_connectionName;
	std::string m_user;
	std::string m_pendingAuthNonce;
	bool m_helloReceived = false;
	bool m_loginReceived = false;
};

}}}

