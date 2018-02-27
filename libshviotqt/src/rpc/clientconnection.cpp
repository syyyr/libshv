#include "clientconnection.h"
#include "socketrpcconnection.h"

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>

#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>

#define logRpc() shvCDebug("rpc")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

ClientConnection::ClientConnection(QObject *parent)
	: Super(SocketRpcConnection::SyncCalls::Supported, parent)
{
	//setDevice(cp::RpcValue(nullptr));

	QTcpSocket *socket = new QTcpSocket();
	setSocket(socket);

	connect(this, &ClientConnection::socketConnectedChanged, this, &ClientConnection::onSocketConnectedChanged);
	//connect(this, &ClientConnection::rpcMessageReceived, this, &ClientConnection::onRpcValueReceived);
	//setProtocolVersion(protocolVersion());
	/*
	{
		m_rpcConnection = new shv::iotqt::chainpack::RpcConnection(cpq::RpcConnection::SyncCalls::Supported, this);
		m_rpcConnection->setSocket(socket);
		m_rpcConnection->setProtocolVersion(protocolVersion());
	}
	*/
}

ClientConnection::~ClientConnection()
{
	shvDebug() << Q_FUNC_INFO;
	abort();
}

void ClientConnection::onSocketConnectedChanged(bool is_connected)
{
	setBrokerConnected(false);
	if(is_connected) {
		shvInfo() << "Socket connected to RPC server";
		//sendKnockKnock(cp::RpcDriver::ChainPack);
		//RpcResponse resp = callMethodSync("echo", "ahoj babi");
		//shvInfo() << "+++" << resp.toStdString();
		sendHello();
	}
	else {
		shvInfo() << "Socket disconnected from RPC server";
	}
}

void ClientConnection::sendHello()
{
	setBrokerConnected(false);
	m_helloRequestId = callMethodASync(cp::Rpc::METH_HELLO);
									   //cp::RpcValue::Map{{"profile", profile()}
																//, {"deviceId", deviceId()}
																//, {"protocolVersion", protocolVersion()}
									   //});
	QTimer::singleShot(5000, this, [this]() {
		if(!isBrokerConnected()) {
			shvError() << "Login time out! Dropping client connection.";
			this->deleteLater();
		}
	});
}

void ClientConnection::sendLogin(const shv::chainpack::RpcValue &server_hello)
{
	m_loginRequestId = callMethodASync(cp::Rpc::METH_LOGIN, createLoginParams(server_hello));
}

std::string ClientConnection::passwordHash(const std::string &user)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(user.data(), user.size());
	QByteArray sha1 = hash.result().toHex();
	return std::string(sha1.constData(), sha1.length());
}

bool ClientConnection::onRpcValueReceived(const cp::RpcValue &val)
{
	if(Super::onRpcValueReceived(val))
		return true;
	cp::RpcMessage msg(val);
	logRpc() << msg.toCpon();
	if(isInitPhase()) {
		processInitPhase(msg);
		return true;
	}
	emit rpcMessageReceived(msg);
	return true;
}

void ClientConnection::processInitPhase(const chainpack::RpcMessage &msg)
{
	do {
		if(!msg.isResponse())
			break;
		cp::RpcResponse resp(msg);
		shvInfo() << "Handshake response received:" << resp.toCpon();
		if(resp.isError())
			break;
		unsigned id = resp.id();
		if(id == 0)
			break;
		if(m_helloRequestId == id) {
			sendLogin(resp.result());
			return;
		}
		else if(m_loginRequestId == id) {
			setBrokerConnected(true);
			return;
		}
	} while(false);
	shvError() << "Invalid handshake message! Dropping connection." << msg.toCpon();
	this->deleteLater();
}

chainpack::RpcValue ClientConnection::createLoginParams(const chainpack::RpcValue &server_hello)
{
	std::string server_nonce = server_hello.toMap().value("nonce").toString();
	std::string password = server_nonce + passwordHash(user());
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(password.c_str(), password.length());
	QByteArray sha1 = hash.result().toHex();
	return cp::RpcValue::Map {
		{"login", cp::RpcValue::Map {
			 {"user", user()},
			 {"password", std::string(sha1.constData())},
		 }
		},
		{"device", device()},
	};
}

}}}


