#include "clientconnection.h"

#include "clientappclioptions.h"
#include "rpc.h"
#include "socketrpcconnection.h"

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>

#include <shv/chainpack/cponreader.h>
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

	setConnectionType(cp::Rpc::TYPE_CLIENT);

	m_rpcDriver = new SocketRpcDriver();
	m_rpcDriver->setProtocolType(cp::Rpc::ProtocolType::ChainPack);
	m_connectionId = m_rpcDriver->connectionId();

	connect(this, &ClientConnection::setProtocolTypeRequest, m_rpcDriver, &SocketRpcDriver::setProtocolTypeAsInt);
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
	m_checkConnectedTimer->setInterval(10*1000);
	connect(m_checkConnectedTimer, &QTimer::timeout, this, &ClientConnection::checkBrokerConnected);
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

void ClientConnection::setCliOptions(const ClientAppCliOptions *cli_opts)
{
	if(!cli_opts)
		return;

	setCheckBrokerConnectedInterval(cli_opts->reconnectInterval() * 1000);

	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	QString pv = cli_opts->protocolType();
	if(pv == QLatin1String("cpon"))
		setProtocolType(shv::chainpack::Rpc::ProtocolType::Cpon);
	else if(pv == QLatin1String("jsonrpc"))
		setProtocolType(shv::chainpack::Rpc::ProtocolType::JsonRpc);
	else
		setProtocolType(shv::chainpack::Rpc::ProtocolType::ChainPack);

	setHost(cli_opts->serverHost().toStdString());
	setPort(cli_opts->serverPort());
	setUser(cli_opts->user().toStdString());
	setPassword(cli_opts->password().toStdString());

	{
		cp::RpcValue::Map opts;
		opts[cp::Rpc::OPT_IDLE_WD_TIMEOUT] = 3 * cli_opts->heartbeatInterval();
		setConnectionOptions(opts);
	}
	int hbi = cli_opts->heartbeatInterval();
	if(hbi > 0) {
		if(!m_pingTimer) {
			shvInfo() << "Creating heart-beat timer, interval:" << hbi << "sec.";
			m_pingTimer = new QTimer(this);
			m_pingTimer->setInterval(hbi * 1000);
			connect(m_pingTimer, &QTimer::timeout, this, [this]() {
				if(m_connectionState.pingRqId > 0) {
					shvError() << "PING response not received within" << (m_pingTimer->interval() / 1000) << "seconds, restarting conection to broker.";
					resetConnection();
				}
				else {
					m_connectionState.pingRqId = callShvMethod(".broker/app", cp::Rpc::METH_PING);
				}
			});
		}
	}
}

void ClientConnection::open()
{
	if(!m_rpcDriver->hasSocket()) {
		QTcpSocket *socket = new QTcpSocket();
		setSocket(socket);
	}
	checkBrokerConnected();
	if(m_checkBrokerConnectedInterval > 0)
		m_checkConnectedTimer->start(m_checkBrokerConnectedInterval);
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

void ClientConnection::setCheckBrokerConnectedInterval(unsigned ms)
{
	m_checkBrokerConnectedInterval = ms;
	if(ms == 0)
		m_checkConnectedTimer->stop();
	else
		m_checkConnectedTimer->setInterval(ms);
}

void ClientConnection::resetConnection()
{
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
	const cp::RpcValue::UInt id = msg.requestId().toUInt();
	if(m_syncCalls == SyncCalls::Enabled && id > 0 && id <= m_connectionState.maxSyncMessageId) {
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
	m_connectionState.maxSyncMessageId = qMax(m_connectionState.maxSyncMessageId, rpc_request_message.requestId().toUInt());
	//logRpcSyncCalls() << "==> send SYNC MSG id:" << rpc_request_message.id() << "data:" << rpc_request_message.toStdString();
	emit sendMessageSyncRequest(rpc_request_message, &res_msg, time_out_ms);
	//logRpcSyncCalls() << "<== RESP SYNC MSG id:" << res_msg.id() << "data:" << res_msg.toStdString();
	return res_msg;
}

void ClientConnection::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	//logRpcMsg() << msg.toCpon();
	if(isInitPhase()) {
		processInitPhase(msg);
		return;
	}
	if(msg.isResponse()) {
		cp::RpcResponse rp(msg);
		if(rp.requestId() == m_connectionState.pingRqId) {
			m_connectionState.pingRqId = 0;
			return;
		}
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
	m_connectionState.helloRequestId = callMethod(cp::Rpc::METH_HELLO);
}

void ClientConnection::sendLogin(const shv::chainpack::RpcValue &server_hello)
{
	m_connectionState.loginRequestId = callMethod(cp::Rpc::METH_LOGIN, createLoginParams(server_hello));
}

std::string ClientConnection::passwordHash(const std::string &user)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	std::string pass = password();
	if(pass.empty())
		pass = user;
	hash.addData(pass.data(), pass.size());
	QByteArray sha1 = hash.result().toHex();
	//shvWarning() << user << pass << sha1;
	return std::string(sha1.constData(), sha1.length());
}

void ClientConnection::processInitPhase(const chainpack::RpcMessage &msg)
{
	do {
		if(!msg.isResponse())
			break;
		cp::RpcResponse resp(msg);
		//shvInfo() << "Handshake response received:" << resp.toCpon();
		if(resp.isError())
			break;
		unsigned id = resp.requestId().toUInt();
		if(id == 0)
			break;
		if(m_connectionState.helloRequestId == id) {
			sendLogin(resp.result());
			return;
		}
		else if(m_connectionState.loginRequestId == id) {
			m_connectionState.loginResult = resp.result();
			setBrokerConnected(true);
			return;
		}
	} while(false);
	shvError() << "Invalid handshake message! Dropping connection." << msg.toCpon();
	resetConnection();
}

chainpack::RpcValue ClientConnection::createLoginParams(const chainpack::RpcValue &server_hello)
{
	std::string server_nonce = server_hello.toMap().value("nonce").toString();
	std::string password = server_nonce + passwordHash(user());
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(password.c_str(), password.length());
	QByteArray sha1 = hash.result().toHex();
	//shvWarning() << password << sha1;
	return cp::RpcValue::Map {
		{"login", cp::RpcValue::Map {
			 {"user", user()},
			 {"password", std::string(sha1.constData())},
		 },
		},
		{"type", connectionType()},
		{"options", connectionOptions()},
	};
}

void ClientConnection::checkBrokerConnected()
{
	//shvWarning() << "check: " << isSocketConnected();
	if(!isBrokerConnected()) {
		emit abortConnectionRequest();
		shvInfo().nospace() << "connecting to: " << user() << "@" << host() << ":" << port();
		m_connectionState = ConnectionState();
		emit connectToHostRequest(QString::fromStdString(host()), port());
	}
}

void ClientConnection::setBrokerConnected(bool b)
{
	if(b != m_connectionState.isBrokerConnected) {
		m_connectionState.isBrokerConnected = b;
		if(b) {
			if(m_pingTimer && m_pingTimer->interval() > 0)
				//m_connStatus.pingRqId = 0;
				m_pingTimer->start();
		}
		else {
			if(m_pingTimer)
				m_pingTimer->stop();
		}
		emit brokerConnectedChanged(b);
	}
}

unsigned ClientConnection::brokerClientId() const
{
	return loginResult().toMap().value(cp::Rpc::KEY_CLIENT_ID).toUInt();
}

}}}


