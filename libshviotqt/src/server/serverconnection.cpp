#include "serverconnection.h"
//#include "../brokerapp.h"

#include <shv/coreqt/chainpack/rpcconnection.h>
#include <shv/coreqt/log.h>

#include <shv/core/exception.h>

//#include <shv/chainpack/chainpackprotocol.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;
namespace cpq = shv::coreqt::chainpack;

namespace shv {
namespace iotqt {
namespace server {

static int s_connectionId = 0;

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(parent)
	//, m_socket(socket)
	, m_connectionId(++s_connectionId)
{
	setSocket(socket);
	socket->setParent(nullptr);
	connect(this, &ServerConnection::socketConnectedChanged, [this](bool is_connected) {
		if(!is_connected) {
			this->deleteLater();
		}
		else {
			QString cn = QStringLiteral("%1:%2").arg(peerAddress()).arg(peerPort());
			setConnectionName(cn.toStdString());
		}
	});
}

ServerConnection::~ServerConnection()
{
	shvInfo() << "Agent disconnected:" << connectionName();
}

namespace {
int nextRpcId()
{
	static int n = 0;
	return ++n;
}
}
int ServerConnection::callMethodASync(const std::string & method, const cp::RpcValue &params)
{
	int id = nextRpcId();
	cp::RpcRequest rq;
	rq.setId(id);
	rq.setMethod(method);
	rq.setParams(params);
	sendMessage(rq.value());
	return id;
}

void ServerConnection::sendResponse(int request_id, const cp::RpcValue &result)
{
	cp::RpcResponse resp;
	resp.setId(request_id);
	resp.setResult(result);
	sendMessage(resp.value());
}

void ServerConnection::sendError(int request_id, const cp::RpcResponse::Error &error)
{
	cp::RpcResponse resp;
	resp.setId(request_id);
	resp.setError(error);
	sendMessage(resp.value());
}

QString ServerConnection::peerAddress() const
{
	if(m_socket)
		return m_socket->peerAddress().toString();
	return QString();
}

int ServerConnection::peerPort() const
{
	if(m_socket)
		return m_socket->peerPort();
	return -1;
}

void ServerConnection::onRpcDataReceived(chainpack::Rpc::ProtocolVersion protocol_version, chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	if(isLoginPhase()) {
		chainpack::RpcValue rpc_val = decodeData(protocol_version, data, start_pos);
		rpc_val.setMetaData(std::move(md));
		cp::RpcMessage msg(rpc_val);
		logRpcMsg() << msg.toCpon();
		cp::RpcRequest rq(msg);
		try {
			if(!msg.isRequest())
				SHV_EXCEPTION("Initial message is not RPC request! Dropping client connection. " + connectionName() + " " + msg.toCpon());
			//shvInfo() << "RPC request received:" << rq.toStdString();
			/*
				if(!m_helloReceived && !m_loginReceived && rq.method() == "echo" && isEchoEnabled()) {
					shvInfo() << "Client ECHO request received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
					sendResponse(rq.requestId(), rq.params());
					return;
				}
				if(rq.method() == "ping") {
					shvInfo() << "Client PING request received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
					sendResponse(rq.requestId(), true);
					return;
				}
				*/
			if(!m_helloReceived && !m_loginReceived && rq.method() == shv::chainpack::Rpc::METH_HELLO) {
				shvInfo() << "Client hello received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
				//const shv::chainpack::RpcValue::String profile = m.value("profile").toString();
				//m_profile = profile;
				m_helloReceived = true;
				shvInfo() << "sending hello response:" << connectionName();// << "profile:" << m_profile;
				m_pendingAuthNonce = std::to_string(std::rand());
				cp::RpcValue::Map params {
					//{"protocol", cp::RpcValue::Map{{"version", protocol_version}}},
					{"nonce", m_pendingAuthNonce}
				};
				sendResponse(rq.id(), params);
				QTimer::singleShot(3000, this, [this]() {
					if(!m_loginReceived) {
						shvError() << "client login time out! Dropping client connection." << connectionName();
						this->deleteLater();
					}
				});
				return;
			}
			if(m_helloReceived && !m_loginReceived && rq.method() == shv::chainpack::Rpc::METH_LOGIN) {
				shvInfo() << "Client login received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
				m_loginReceived = true;
				//cp::RpcValue::Map params = rq.params().toMap();
				//const cp::RpcValue login = params.value("login");
				if(!login(rq.params()))
					SHV_EXCEPTION("Invalid authentication for user: " + m_user + " at: " + connectionName());
				shvInfo() << "Client logged in user:" << m_user << "from:" << connectionName();
				sendResponse(rq.id(), true);
				return;
			}
		}
		catch(shv::core::Exception &e) {
			sendError(rq.id(), cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
		}
		shvError() << "Initial handshake error! Dropping client connection." << connectionName() << msg.toCpon();
		QTimer::singleShot(100, this, &ServerConnection::deleteLater); // need some time to send error to client
		return;
	}
	Super::onRpcDataReceived(protocol_version, std::move(md), data, start_pos, data_len);
}

}}}


