#include "clientconnection.h"
#include "rpc.h"
#include "socketrpcconnection.h"

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>

#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>
#include <QThread>

#define logRpcMsg() shvCDebug("RpcMsg")
#define logRpcSyncCalls() shvCDebug("RpcSyncCalls")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

ClientConnection::ClientConnection(SyncCalls sync_calls, QObject *parent)
	: QObject(parent)
	, m_syncCalls(sync_calls)
{
	Rpc::registerMetatTypes();

	static int id = 0;
	m_connectionId = ++id;
	m_rpcDriver = new SocketRpcDriver();

	connect(this, &ClientConnection::setProtocolVersionRequest, m_rpcDriver, &SocketRpcDriver::setProtocolVersionAsInt);
	connect(this, &ClientConnection::sendMessageRequest, m_rpcDriver, &SocketRpcDriver::sendRpcValue);
	connect(this, &ClientConnection::connectToHostRequest, m_rpcDriver, &SocketRpcDriver::connectToHost);
	connect(this, &ClientConnection::closeConnectionRequest, m_rpcDriver, &SocketRpcDriver::closeConnection);
	connect(this, &ClientConnection::abortConnectionRequest, m_rpcDriver, &SocketRpcDriver::abortConnection);

	connect(m_rpcDriver, &SocketRpcDriver::socketConnectedChanged, this, &ClientConnection::socketConnectedChanged);
	connect(m_rpcDriver, &SocketRpcDriver::rpcValueReceived, this, &ClientConnection::onRpcValueReceived);

	if(m_syncCalls == SyncCalls::Enabled) {
		connect(this, &ClientConnection::sendMessageSyncRequest, m_rpcDriver, &SocketRpcDriver::sendRpcRequestSync_helper, Qt::BlockingQueuedConnection);
		m_rpcDriverThread = new QThread();
		m_rpcDriver->moveToThread(m_rpcDriverThread);
		m_rpcDriverThread->start();
	}
	else {
		connect(this, &ClientConnection::sendMessageSyncRequest, []() {
			shvError() << "Sync calls are enabled in threaded RPC connection only!";
		});
	}

	connect(this, &ClientConnection::socketConnectedChanged, this, &ClientConnection::onSocketConnectedChanged);

	m_checkConnectedTimer = new QTimer(this);
	m_checkConnectedTimer->setInterval(1000 * 10);
	connect(m_checkConnectedTimer, &QTimer::timeout, this, &ClientConnection::checkConnected);
}

ClientConnection::~ClientConnection()
{
	shvDebug() << __FUNCTION__;
	abort();
	if(m_syncCalls == SyncCalls::Enabled) {
		if(m_rpcDriverThread->isRunning()) {
			shvDebug() << "stopping rpc driver thread";
			m_rpcDriverThread->quit();
			shvDebug() << "after quit";
		}
		shvDebug() << "joining rpc driver thread";
		bool ok = m_rpcDriverThread->wait();
		shvDebug() << "rpc driver thread joined ok:" << ok;
		delete m_rpcDriverThread;
	}
	delete m_rpcDriver;
}

void ClientConnection::open()
{
	if(!m_rpcDriver->hasSocket()) {
		QTcpSocket *socket = new QTcpSocket();
		setSocket(socket);
	}
	checkConnected();
	m_checkConnectedTimer->start();
}

void ClientConnection::close()
{
	m_checkConnectedTimer->stop();
	emit closeConnectionRequest();
}

void ClientConnection::abort()
{
	m_checkConnectedTimer->stop();
	emit abortConnectionRequest();
}

void ClientConnection::setSocket(QTcpSocket *socket)
{
	m_rpcDriver->setSocket(socket);
}

bool ClientConnection::isSocketConnected() const
{
	return m_rpcDriver->isSocketConnected();
}

void ClientConnection::onRpcValueReceived(const shv::chainpack::RpcValue &val)
{
	cp::RpcMessage msg(val);
	cp::RpcValue::UInt id = msg.requestId();
	if(id > 0 && id <= m_maxSyncMessageId) {
		// ignore messages alredy processed by sync calls
		logRpcSyncCalls() << "XXX ignoring already served sync response:" << id;
		return;
	}
	//logRpcMsg() << cp::RpcDriver::RCV_LOG_ARROW << msg.toStdString();
	onRpcMessageReceived(msg);
}

void ClientConnection::sendMessage(const cp::RpcMessage &rpc_msg)
{
	//logRpcMsg() << cp::RpcDriver::SND_LOG_ARROW << rpc_msg.toStdString();
	emit sendMessageRequest(rpc_msg.value());
}

cp::RpcResponse ClientConnection::sendMessageSync(const cp::RpcRequest &rpc_request_message, int time_out_ms)
{
	cp::RpcResponse res_msg;
	m_maxSyncMessageId = qMax(m_maxSyncMessageId, rpc_request_message.requestId());
	//logRpcSyncCalls() << "==> send SYNC MSG id:" << rpc_request_message.id() << "data:" << rpc_request_message.toStdString();
	emit sendMessageSyncRequest(rpc_request_message, &res_msg, time_out_ms);
	//logRpcSyncCalls() << "<== RESP SYNC MSG id:" << res_msg.id() << "data:" << res_msg.toStdString();
	return res_msg;
}

void ClientConnection::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	logRpcMsg() << msg.toCpon();
	if(isInitPhase()) {
		processInitPhase(msg);
		return;
	}
	emit rpcMessageReceived(msg);
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
	m_helloRequestId = callMethod(cp::Rpc::METH_HELLO);
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
	m_loginRequestId = callMethod(cp::Rpc::METH_LOGIN, createLoginParams(server_hello));
}

std::string ClientConnection::passwordHash(const std::string &user)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(user.data(), user.size());
	QByteArray sha1 = hash.result().toHex();
	return std::string(sha1.constData(), sha1.length());
}
/*
void ClientConnection::connectToHost(const std::string &host_name, quint16 port)
{
	emit connectToHostRequest(QString::fromStdString(host_name), port);
}
*/

void ClientConnection::processInitPhase(const chainpack::RpcMessage &msg)
{
	do {
		if(!msg.isResponse())
			break;
		cp::RpcResponse resp(msg);
		shvInfo() << "Handshake response received:" << resp.toCpon();
		if(resp.isError())
			break;
		unsigned id = resp.requestId();
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
	};
}

void ClientConnection::checkConnected()
{
	if(!isSocketConnected()) {
		abort();
		shvInfo().nospace() << "connecting to: " << user() << "@" << host() << ":" << port();
		//m_clientConnection->setProtocolVersion(protocolVersion());
		emit connectToHostRequest(QString::fromStdString(host()), port());
	}
}

}}}


