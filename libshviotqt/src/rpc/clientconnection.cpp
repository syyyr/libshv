#include "clientconnection.h"

#include "clientappclioptions.h"
#include "rpc.h"
#include "socket.h"
#include "socketrpcconnection.h"

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>
#include <shv/core/string.h>

#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QSslSocket>
#include <QHostAddress>
#include <QTimer>
#include <QCryptographicHash>
#include <QThread>

#include <fstream>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

ClientConnection::ClientConnection(QObject *parent)
	: Super(parent)
	, m_loginType(IRpcConnection::LoginType::Sha1)
{
	setProtocolType(shv::chainpack::Rpc::ProtocolType::ChainPack);

	connect(this, &SocketRpcConnection::socketConnectedChanged, this, &ClientConnection::onSocketConnectedChanged);

	m_checkConnectedTimer = new QTimer(this);
	connect(m_checkConnectedTimer, &QTimer::timeout, this, &ClientConnection::checkBrokerConnected);
}

ClientConnection::~ClientConnection()
{
	shvDebug() << __FUNCTION__;
	abort();
}

ClientConnection::SecurityType ClientConnection::securityTypeFromString(const std::string &val)
{
	return val == "ssl" ? SecurityType::Ssl : SecurityType::None;
}

std::string ClientConnection::securityTypeToString(const SecurityType &security_type)
{
	switch (security_type) {
	case SecurityType::None:
		return "none";
	case SecurityType::Ssl:
		return "ssl";
	}
	return std::string();
}

ClientConnection::SecurityType ClientConnection::securityType() const
{
	return m_securityType;
}

void ClientConnection::setSecurityType(SecurityType type)
{
	m_securityType = type;
}

void ClientConnection::setSecurityType(const std::string &val)
{
	m_securityType = securityTypeFromString(val);
}

void ClientConnection::setCliOptions(const ClientAppCliOptions *cli_opts)
{
	if(!cli_opts)
		return;

	setCheckBrokerConnectedInterval(cli_opts->reconnectInterval() * 1000);

	//if(cli_opts->isMetaTypeExplicit_isset())
	//	cp::RpcMessage::setMetaTypeExplicit(cli_opts->isMetaTypeExplicit());
	if(cli_opts->defaultRpcTimeout_isset()) {
		cp::RpcDriver::setDefaultRpcTimeoutMsec(cli_opts->defaultRpcTimeout() * 1000);
		shvInfo() << "Default RPC timeout set to:" << cp::RpcDriver::defaultRpcTimeoutMsec() << "msec.";
	}

	const std::string pv = cli_opts->protocolType();
	if(pv == "cpon")
		setProtocolType(shv::chainpack::Rpc::ProtocolType::Cpon);
	else if(pv == "jsonrpc")
		setProtocolType(shv::chainpack::Rpc::ProtocolType::JsonRpc);
	else
		setProtocolType(shv::chainpack::Rpc::ProtocolType::ChainPack);

	setHost(cli_opts->serverHost());
	setPort(cli_opts->serverPort());
	setSecurityType(cli_opts->serverSecurityType());
	setPeerVerify(cli_opts->serverPeerVerify());
	setUser(cli_opts->user());
	setPassword(cli_opts->password());
	if(password().empty() && !cli_opts->passwordFile().empty()) {
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
	shvDebug() << cli_opts->loginType() << "-->" << (int)shv::chainpack::UserLogin::loginTypeFromString(cli_opts->loginType());
	setLoginType(shv::chainpack::UserLogin::loginTypeFromString(cli_opts->loginType()));

	setHeartBeatInterval(cli_opts->heartBeatInterval());
	{
		cp::RpcValue::Map opts;
		opts[cp::Rpc::OPT_IDLE_WD_TIMEOUT] = 3 * heartBeatInterval();
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
		QSslSocket::PeerVerifyMode peer_verify_mode = m_peerVerify ? QSslSocket::AutoVerifyPeer : QSslSocket::VerifyNone;
		TcpSocket *socket = m_securityType == None ? new TcpSocket(new QTcpSocket()) : new SslSocket(new QSslSocket(), peer_verify_mode);
		setSocket(socket);
	}
	checkBrokerConnected();
	if(m_checkBrokerConnectedInterval > 0) {
		shvInfo() << "Starting check-connected timer, interval:" << m_checkBrokerConnectedInterval/1000 << "sec.";
		m_checkConnectedTimer->start(m_checkBrokerConnectedInterval);
	}
}

void ClientConnection::closeOrAbort(bool is_abort)
{
	m_checkConnectedTimer->stop();
	if(m_socket) {
		if(is_abort)
			abortSocket();
		else
			closeSocket();
		m_socket->deleteLater();
		m_socket = nullptr;
	}
	setState(State::NotConnected);
}

void ClientConnection::setCheckBrokerConnectedInterval(int ms)
{
	m_checkBrokerConnectedInterval = ms;
	if(ms == 0)
		m_checkConnectedTimer->stop();
	else
		m_checkConnectedTimer->setInterval(ms);
}

void ClientConnection::sendMessage(const cp::RpcMessage &rpc_msg)
{
	if(rpc_msg.isSignal() && shv::core::String::endsWith(rpc_msg.shvPath().toString(), "server/time")) {
		// skip annoying messages
	}
	else {
		logRpcMsg() << SND_LOG_ARROW
					<< "client id:" << connectionId()
					<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
					<< rpc_msg.toPrettyString();
	}
	sendRpcValue(rpc_msg.value());
}

void ClientConnection::onRpcMessageReceived(const chainpack::RpcMessage &msg)
{
	if(msg.isSignal() && shv::core::String::endsWith(msg.shvPath().toString(), "server/time")) {
		// skip annoying messages
	}
	else {
		logRpcMsg() << cp::RpcDriver::RCV_LOG_ARROW
					<< "client id:" << connectionId()
					<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
					<< msg.toPrettyString();
	}
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

void ClientConnection::setState(ClientConnection::State state)
{
	if(m_connectionState.state == state)
		return;
	State old_state = m_connectionState.state;
	m_connectionState.state = state;
	emit stateChanged(state);
	if(old_state == State::BrokerConnected)
		whenBrokerConnectedChanged(false);
	else if(state == State::BrokerConnected)
		whenBrokerConnectedChanged(true);
}

void ClientConnection::onRpcValueReceived(const chainpack::RpcValue &rpc_val)
{
	cp::RpcMessage msg(rpc_val);
	onRpcMessageReceived(msg);
}

void ClientConnection::sendHello()
{
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
		abortSocket();
		shvInfo().nospace() << "connecting to: " << user() << "@" << host() << ":" << port() << " security: " << securityTypeToString(securityType());
		m_connectionState = ConnectionState();
		setState(State::Connecting);
		connectToHost(QString::fromStdString(host()), port());
	}
}

void ClientConnection::whenBrokerConnectedChanged(bool b)
{
	if(b) {
		shvInfo() << "Connected to broker" << "client id:" << brokerClientId();// << "mount point:" << brokerMountPoint();
		if(heartBeatInterval() > 0) {
			if(!m_heartBeatTimer) {
				shvInfo() << "Creating heart-beat timer, interval:" << heartBeatInterval() << "sec.";
				m_heartBeatTimer = new QTimer(this);
				m_heartBeatTimer->setInterval(heartBeatInterval() * 1000);
				connect(m_heartBeatTimer, &QTimer::timeout, this, [this]() {
					if(m_connectionState.pingRqId > 0) {
						shvWarning() << "PING response not received within" << (m_heartBeatTimer->interval() / 1000) << "seconds, restarting conection to broker.";
						//restartIfActive();
					}
					else {
						m_connectionState.pingRqId = callShvMethod(".broker/app", cp::Rpc::METH_PING);
					}
				});
			}
			m_heartBeatTimer->start();
		}
		else {
			shvWarning() << "Heart-beat timer interval is set to 0, heart beats will not be sent.";
		}
	}
	else {
		if(m_heartBeatTimer)
			m_heartBeatTimer->stop();
	}
	emit brokerConnectedChanged(b);
}

void ClientConnection::emitInitPhaseError(const std::string &err)
{
	Q_EMIT brokerLoginError(err);
}

void ClientConnection::onSocketConnectedChanged(bool is_connected)
{
	if(is_connected) {
		shvInfo() << "Socket connected to RPC server";
		//sendKnockKnock(cp::RpcDriver::ChainPack);
		//RpcResponse resp = callMethodSync("echo", "ahoj babi");
		//shvInfo() << "+++" << resp.toStdString();
		setState(State::SocketConnected);
		clearBuffers();
		sendHello();
	}
	else {
		shvInfo() << "Socket disconnected from RPC server";
		setState(State::NotConnected);
	}
}

static std::string sha1_hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(s.data(), s.length());
	return std::string(hash.result().toHex().constData());
}

chainpack::RpcValue ClientConnection::createLoginParams(const chainpack::RpcValue &server_hello)
{
	shvDebug() << server_hello.toCpon() << "login type:" << (int)loginType();
	std::string pass;
	if(loginType() == chainpack::IRpcConnection::LoginType::Sha1) {
		std::string server_nonce = server_hello.toMap().value("nonce").toString();
		std::string pwd = password();
		if(pwd.size() == 40)
			shvWarning() << "Using shadowed password directly by client is unsecure and it will be disabled in future SHV versions";
		else
			pwd = sha1_hex(pwd); /// SHA1 password must be 40 chars long, it is considered to be plain if shorter
		std::string pn = server_nonce + pwd;
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
		hash.addData(pn.data(), pn.length());
		QByteArray sha1 = hash.result().toHex();
		pass = std::string(sha1.constData(), sha1.size());
	}
	else if(loginType() == chainpack::IRpcConnection::LoginType::Plain) {
		pass = password();
		shvDebug() << "plain password:" << pass;
	}
	else {
		shvError() << "Login type:" << chainpack::UserLogin::loginTypeToString(loginType()) << "not supported";
	}
	//shvWarning() << password << sha1;
	return cp::RpcValue::Map {
		{"login", cp::RpcValue::Map {
			{"user", user()},
			{"password", pass},
			//{"passwordFormat", chainpack::AbstractRpcConnection::passwordFormatToString(password_format)},
			{"type", chainpack::UserLogin::loginTypeToString(loginType())},
		 },
		},
		{"options", connectionOptions()},
	};
}

int ClientConnection::brokerClientId() const
{
	return m_connectionState.loginResult.toMap().value(cp::Rpc::KEY_CLIENT_ID).toInt();
}

void ClientConnection::processInitPhase(const chainpack::RpcMessage &msg)
{
	do {
		if(!msg.isResponse())
			break;
		cp::RpcResponse resp(msg);
		//shvInfo() << "Handshake response received:" << resp.toCpon();
		if(resp.isError()) {
			setState(State::ConnectionError);
			emitInitPhaseError(resp.error().message());
			break;
		}
		int id = resp.requestId().toInt();
		if(id == 0)
			break;
		if(m_connectionState.helloRequestId == id) {
			sendLogin(resp.result());
			return;
		}
		else if(m_connectionState.loginRequestId == id) {
			m_connectionState.loginResult = resp.result();
			setState(State::BrokerConnected);
			return;
		}
	} while(false);
	shvError() << "Invalid handshake message! Dropping connection." << msg.toCpon();
	if(isAutoConnect())
		setState(State::ConnectionError);
	else
		close();
}

const char *ClientConnection::stateToString(ClientConnection::State state)
{
	switch (state) {
	case State::NotConnected: return "NotConnected";
	case State::Connecting: return "Connecting";
	case State::SocketConnected: return "SocketConnected";
	case State::BrokerConnected: return "BrokerConnected";
	case State::ConnectionError: return "ConnectionError";
	}
	return "this could never happen";
}

}}}


