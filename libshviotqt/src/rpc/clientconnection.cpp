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

#include <fstream>

//#define logRpcMsg() shvCDebug("RpcMsg")
#define logRpcSyncCalls() shvCDebug("RpcSyncCalls")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

#if 0
ConnectionParams::MetaType::MetaType()
	: Super("TunnelParams")
{
	m_keys = {
		RPC_META_KEY_DEF(Host),
		RPC_META_KEY_DEF(Port),
		RPC_META_KEY_DEF(User),
		RPC_META_KEY_DEF(Password),
		RPC_META_KEY_DEF(OnConnectedCall),
		//RPC_META_KEY_DEF(CallerClientIds),
		//RPC_META_KEY_DEF(RequestId),
		//RPC_META_KEY_DEF(TunName),
	};
	//m_tags = {
	//	{(int)Tag::RequestId, {(int)Tag::RequestId, "id"}},
	//	{(int)Tag::ShvPath, {(int)Tag::ShvPath, "shvPath"}},
	//};
}

void ConnectionParams::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

ConnectionParams::ConnectionParams()
	: Super()
{
	MetaType::registerMetaType();
}

ConnectionParams::ConnectionParams(const IMap &m)
	: Super(m)
{
	MetaType::registerMetaType();
}

chainpack::RpcValue ConnectionParams::toRpcValue() const
{
	cp::RpcValue ret(*this);
	ret.setMetaValue(cp::meta::Tag::MetaTypeId, ConnectionParams::MetaType::ID);
	return ret;
}
#endif

ClientConnection::ClientConnection(QObject *parent)
	: Super(parent)
{
	Rpc::registerMetatTypes();

	//setConnectionType(cp::Rpc::TYPE_CLIENT);

	connect(this, &SocketRpcDriver::socketConnectedChanged, this, &ClientConnection::onSocketConnectedChanged);

	m_checkConnectedTimer = new QTimer(this);
	m_checkConnectedTimer->setInterval(10*1000);
	connect(m_checkConnectedTimer, &QTimer::timeout, this, &ClientConnection::checkBrokerConnected);
}

ClientConnection::~ClientConnection()
{
	shvDebug() << __FUNCTION__;
	abort();
}

void ClientConnection::setCliOptions(const ClientAppCliOptions *cli_opts)
{
	if(!cli_opts)
		return;

	setCheckBrokerConnectedInterval(cli_opts->reconnectInterval() * 1000);

	//if(cli_opts->isMetaTypeExplicit_isset())
	//	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());

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
	if(cli_opts->password_isset()) {
		setPassword(cli_opts->password());
	}
	else if(cli_opts->passwordFile_isset()) {
		std::ifstream is(cli_opts->passwordFile(), std::ios::binary);
		if(is) {
			std::string pwd;
			is >> pwd;
			setPassword(pwd);
		}
		else {
			shvError() << "Cannot open password file";
		}
	}
	setLoginType(shv::chainpack::AbstractRpcConnection::loginTypeFromString(cli_opts->loginType()));

	m_heartbeatInterval = cli_opts->heartbeatInterval();
	{
		cp::RpcValue::Map opts;
		opts[cp::Rpc::OPT_IDLE_WD_TIMEOUT] = 3 * m_heartbeatInterval;
		setConnectionOptions(opts);
	}
}

void ClientConnection::setTunnelOptions(const chainpack::RpcValue &opts)
{
	shv::chainpack::RpcValue::Map conn_opts = connectionOptions().toMap();
	conn_opts[cp::Rpc::KEY_TUNNEL] = opts;
	setConnectionOptions(conn_opts);
}

void ClientConnection::open()
{
	if(!hasSocket()) {
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
	closeConnection();
}

void ClientConnection::abort()
{
	m_checkConnectedTimer->stop();
	abortConnection();
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
	abortConnection();
}

void ClientConnection::sendMessage(const cp::RpcMessage &rpc_msg)
{
	//logRpcMsg() << cp::RpcDriver::SND_LOG_ARROW << rpc_msg.toStdString();
	sendRpcValue(rpc_msg.value());
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

void ClientConnection::onRpcValueReceived(const chainpack::RpcValue &rpc_val)
{
	cp::RpcMessage msg(rpc_val);
	onRpcMessageReceived(msg);
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

void ClientConnection::checkBrokerConnected()
{
	//shvWarning() << "check: " << isSocketConnected();
	if(!isBrokerConnected()) {
		abortConnection();
		shvInfo().nospace() << "connecting to: " << user() << "@" << host() << ":" << port();
		m_connectionState = ConnectionState();
		connectToHost(QString::fromStdString(host()), port());
	}
}

void ClientConnection::setBrokerConnected(bool b)
{
	if(b != m_connectionState.isBrokerConnected) {
		m_connectionState.isBrokerConnected = b;
		if(b) {
			shvInfo() << "Connected to broker";// << "client id:" << brokerClientId();// << "mount point:" << brokerMountPoint();
			if(m_heartbeatInterval > 0) {
				if(!m_pingTimer) {
					shvInfo() << "Creating heart-beat timer, interval:" << m_heartbeatInterval << "sec.";
					m_pingTimer = new QTimer(this);
					m_pingTimer->setInterval(m_heartbeatInterval * 1000);
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
				m_pingTimer->start();
			}
		}
		else {
			if(m_pingTimer)
				m_pingTimer->stop();
		}
		emit brokerConnectedChanged(b);
	}
}

void ClientConnection::onSocketConnectedChanged(bool is_connected)
{
	IClientConnection::onSocketConnectedChanged(is_connected);
}

}}}


