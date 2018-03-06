#include "serverconnection.h"
#include "socketrpcconnection.h"

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>

//#include <shv/chainpack/chainpackprotocol.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QTimer>
#include <QCryptographicHash>

#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(parent)
{
	setSocket(socket);
	socket->setParent(nullptr);
	connect(this, &ServerConnection::socketConnectedChanged, [this](bool is_connected) {
		if(is_connected) {
			setConnectionName(peerAddress() + ':' + std::to_string(peerPort()));
		}
	});
}

ServerConnection::~ServerConnection()
{
	shvInfo() << "Agent disconnected:" << connectionName();
}

void ServerConnection::sendMessage(const chainpack::RpcMessage &rpc_msg)
{
	sendRpcValue(rpc_msg.value());
}

chainpack::RpcResponse ServerConnection::sendMessageSync(const chainpack::RpcRequest &rpc_request, int time_out_ms)
{
	Q_UNUSED(rpc_request)
	Q_UNUSED(time_out_ms)
	SHV_EXCEPTION("Sync mesage are nott supported by server connection!");
	return chainpack::RpcResponse();
}

void ServerConnection::onRpcDataReceived(shv::chainpack::Rpc::ProtocolVersion protocol_version, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	if(isInitPhase()) {
		shv::chainpack::RpcValue rpc_val = decodeData(protocol_version, data, start_pos);
		rpc_val.setMetaData(std::move(md));
		cp::RpcMessage msg(rpc_val);
		logRpcMsg() << msg.toCpon();
		processInitPhase(msg);
		return;
	}
	Super::onRpcDataReceived(protocol_version, std::move(md), data, start_pos, data_len);
}

void ServerConnection::onRpcValueReceived(const chainpack::RpcValue &rpc_val)
{
	cp::RpcMessage msg(rpc_val);
	onRpcMessageReceived(msg);
}

void ServerConnection::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	emit rpcMessageReceived(msg);
}

void ServerConnection::processInitPhase(const chainpack::RpcMessage &msg)
{
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
			sendResponse(rq.requestId(), params);
			QTimer::singleShot(3000, this, [this]() {
				if(!m_loginReceived) {
					shvError() << "client login time out! Dropping client connection." << connectionName();
					abort();
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
			shvInfo().nospace() << "Client logged in user: " << m_user << " from: " << peerAddress() << ':' << peerPort();
			sendResponse(rq.requestId(), true);
			return;
		}
	}
	catch(shv::core::Exception &e) {
		sendError(rq.requestId(), cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.message()));
	}
	shvError() << "Initial handshake error! Dropping client connection." << connectionName() << msg.toCpon();
	QTimer::singleShot(100, this, &ServerConnection::abort); // need some time to send error to client
}

}}}


