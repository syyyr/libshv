#include "syncclientconnection.h"

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

//#define logRpcMsg() shvCDebug("RpcMsg")
#define logRpcSyncCalls() shvCDebug("RpcSyncCalls")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

SyncClientConnection::SyncClientConnection(QObject *parent)
	: QObject(parent)
{
	Rpc::registerMetaTypes();

	//setConnectionType(cp::Rpc::TYPE_CLIENT);

	m_rpcDriver = new SocketRpcDriver();
	m_rpcDriver->setProtocolType(cp::Rpc::ProtocolType::ChainPack);

	connect(this, &SyncClientConnection::setProtocolTypeRequest, m_rpcDriver, &SocketRpcDriver::setProtocolTypeAsInt);
	connect(this, &SyncClientConnection::sendMessageRequest, m_rpcDriver, &SocketRpcDriver::sendRpcValue);
	connect(this, &SyncClientConnection::connectToHostRequest, m_rpcDriver, &SocketRpcDriver::connectToHost);
	connect(this, &SyncClientConnection::closeConnectionRequest, m_rpcDriver, &SocketRpcDriver::closeConnection);
	connect(this, &SyncClientConnection::abortConnectionRequest, m_rpcDriver, &SocketRpcDriver::abortConnection);

	connect(m_rpcDriver, &SocketRpcDriver::socketConnectedChanged, this, &SyncClientConnection::socketConnectedChanged);
	connect(m_rpcDriver, &SocketRpcDriver::rpcValueReceived, this, &SyncClientConnection::onRpcValueReceived);

	shvInfo() << "Creating RPC thread to support SYNC calls.";
	connect(this, &SyncClientConnection::sendMessageSyncRequest, m_rpcDriver, &SocketRpcDriver::sendRpcRequestSync_helper, Qt::BlockingQueuedConnection);
	m_rpcDriverThread = new QThread();
	m_rpcDriver->moveToThread(m_rpcDriverThread);
	m_rpcDriverThread->start();

	connect(this, &SyncClientConnection::socketConnectedChanged, this, &SyncClientConnection::onSocketConnectedChanged);

	m_checkConnectedTimer = new QTimer(this);
	m_checkConnectedTimer->setInterval(10*1000);
	connect(m_checkConnectedTimer, &QTimer::timeout, this, &SyncClientConnection::checkBrokerConnected);
}

SyncClientConnection::~SyncClientConnection()
{
	shvDebug() << __FUNCTION__;
	abort();
	if(m_rpcDriverThread->isRunning()) {
		shvDebug() << "stopping rpc driver thread";
		m_rpcDriverThread->quit();
		shvDebug() << "after quit";
	}
	shvDebug() << "joining rpc driver thread";
	bool ok = m_rpcDriverThread->wait();
	shvDebug() << "rpc driver thread joined ok:" << ok;
	delete m_rpcDriverThread;
	delete m_rpcDriver;
}

void SyncClientConnection::setCliOptions(const ClientAppCliOptions *cli_opts)
{
	if(!cli_opts)
		return;

	setCheckBrokerConnectedInterval(cli_opts->reconnectInterval() * 1000);

	//cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

	const std::string pv = cli_opts->protocolType();
	if(pv == "cpon")
		setProtocolType(shv::chainpack::Rpc::ProtocolType::Cpon);
	else if(pv == "jsonrpc")
		setProtocolType(shv::chainpack::Rpc::ProtocolType::JsonRpc);
	else
		setProtocolType(shv::chainpack::Rpc::ProtocolType::ChainPack);

	setHost(cli_opts->serverHost());
	setPort(cli_opts->serverPort());
	setUser(cli_opts->user());
	setPassword(cli_opts->password());

	{
		cp::RpcValue::Map opts;
		opts[cp::Rpc::OPT_IDLE_WD_TIMEOUT] = 3 * cli_opts->heartbeatInterval();
		setConnectionOptions(opts);
	}
	int hbi = cli_opts->heartbeatInterval();
	if(hbi > 0) {
		if(!m_pingTimer) {
			shvInfo() << "Preparing heart-beat timer, interval:" << hbi << "sec.";
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

void SyncClientConnection::open()
{
	if(!m_rpcDriver->hasSocket()) {
		QTcpSocket *socket = new QTcpSocket();
		setSocket(socket);
	}
	checkBrokerConnected();
	if(m_checkBrokerConnectedInterval > 0)
		m_checkConnectedTimer->start(m_checkBrokerConnectedInterval);
}

void SyncClientConnection::close()
{
	m_checkConnectedTimer->stop();
	emit closeConnectionRequest();
}

void SyncClientConnection::abort()
{
	m_checkConnectedTimer->stop();
	emit abortConnectionRequest();
}

void SyncClientConnection::setCheckBrokerConnectedInterval(unsigned ms)
{
	m_checkBrokerConnectedInterval = ms;
	if(ms == 0)
		m_checkConnectedTimer->stop();
	else
		m_checkConnectedTimer->setInterval(ms);
}

void SyncClientConnection::resetConnection()
{
	emit abortConnectionRequest();
}

void SyncClientConnection::setSocket(QTcpSocket *socket)
{
	m_rpcDriver->setSocket(socket);
}

bool SyncClientConnection::isSocketConnected() const
{
	return m_rpcDriver->isSocketConnected();
}

void SyncClientConnection::onRpcValueReceived(const shv::chainpack::RpcValue &val)
{
	cp::RpcMessage msg(val);
	const cp::RpcValue::UInt id = msg.requestId().toUInt();
	if(id > 0 && id <= m_maxSyncMessageId) {
		// ignore messages alredy processed by sync calls
		logRpcSyncCalls() << "XXX ignoring already served sync response:" << id;
		return;
	}
	//logRpcMsg() << cp::RpcDriver::RCV_LOG_ARROW << msg.toStdString();
	onRpcMessageReceived(msg);
}

void SyncClientConnection::sendMessage(const cp::RpcMessage &rpc_msg)
{
	//logRpcMsg() << cp::RpcDriver::SND_LOG_ARROW << rpc_msg.toStdString();
	emit sendMessageRequest(rpc_msg.value());
}

cp::RpcResponse SyncClientConnection::sendMessageSync(const cp::RpcRequest &rpc_request_message, int time_out_ms)
{
	cp::RpcResponse res_msg;
	m_maxSyncMessageId = qMax(m_maxSyncMessageId, rpc_request_message.requestId().toUInt());
	//logRpcSyncCalls() << "==> send SYNC MSG id:" << rpc_request_message.id() << "data:" << rpc_request_message.toStdString();
	emit sendMessageSyncRequest(rpc_request_message, &res_msg, time_out_ms);
	//logRpcSyncCalls() << "<== RESP SYNC MSG id:" << res_msg.id() << "data:" << res_msg.toStdString();
	return res_msg;
}

void SyncClientConnection::onRpcMessageReceived(const chainpack::RpcMessage &msg)
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

void SyncClientConnection::onSocketConnectedChanged(bool is_connected)
{
	IClientConnection::onSocketConnectedChanged(is_connected);
}

void SyncClientConnection::sendHello()
{
	setBrokerConnected(false);
	m_connectionState.helloRequestId = callMethod(cp::Rpc::METH_HELLO);
}

void SyncClientConnection::sendLogin(const shv::chainpack::RpcValue &server_hello)
{
	m_connectionState.loginRequestId = callMethod(cp::Rpc::METH_LOGIN, createLoginParams(server_hello));
}

void SyncClientConnection::checkBrokerConnected()
{
	//shvWarning() << "check: " << isSocketConnected();
	if(!isBrokerConnected()) {
		emit abortConnectionRequest();
		shvInfo().nospace() << "connecting to: " << user() << "@" << host() << ":" << port();
		m_connectionState = ConnectionState();
		emit connectToHostRequest(QString::fromStdString(host()), port());
	}
}

void SyncClientConnection::setBrokerConnected(bool b)
{
	if(b != m_connectionState.isBrokerConnected) {
		m_connectionState.isBrokerConnected = b;
		if(b) {
			shvInfo() << "Connected to broker";// << "client id:" << brokerClientId();// << "mount point:" << brokerMountPoint();
			if(m_pingTimer && m_pingTimer->interval() > 0)
				//m_connStatus.pingRqId = 0;
				m_pingTimer->start();
		}
		else {
			if(m_pingTimer)
				m_pingTimer->stop();
		}
		emit brokerConnectedChanged(b);
	}}

static int s_defaultRpcTimeout = 5000;

int SyncClientConnection::defaultRpcTimeout()
{
	return s_defaultRpcTimeout;
}

int SyncClientConnection::setDefaultRpcTimeout(int rpc_timeout)
{
	int old = s_defaultRpcTimeout;
	s_defaultRpcTimeout = rpc_timeout;
	return old;
}

/*
unsigned SynClientConnection::brokerClientId() const
{
	return loginResult().value(cp::Rpc::KEY_CLIENT_ID).toUInt();
}

std::string SynClientConnection::brokerMountPoint() const
{
	return loginResult().value(cp::Rpc::KEY_MOUT_POINT).toString();
}
*/
}}}


