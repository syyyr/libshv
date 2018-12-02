#include "serverconnection.h"
#include "socketrpcconnection.h"

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>
#include <shv/core/string.h>

//#include <shv/chainpack/chainpackprotocol.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTcpSocket>
#include <QTimer>
#include <QCryptographicHash>
#include <QHostAddress>

//#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

static int s_initPhaseTimeout = 10000;

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(parent)
{
	//socket->setParent(nullptr);
	setSocket(socket);
	connect(this, &ServerConnection::socketConnectedChanged, [this](bool is_connected) {
		if(is_connected) {
			m_helloReceived = m_loginReceived = false;
			setConnectionName(peerAddress() + ':' + shv::chainpack::Utils::toString(peerPort()));
		}
	});
	QTimer::singleShot(s_initPhaseTimeout, this, [this]() {
		if(isInitPhase()) {
			shvWarning() << "Client should login in" << (s_initPhaseTimeout/1000) << "seconds, dropping out connection.";
			abort();
		}
	});
}

ServerConnection::~ServerConnection()
{
	shvInfo() << "Destroying Connection ID:" << connectionId() << "name:" << connectionName();
	abort();
}

bool ServerConnection::isSlaveBrokerConnection() const
{
	return m_connectionOptions.toMap().hasKey(cp::Rpc::KEY_BROKER);
}

void ServerConnection::sendMessage(const chainpack::RpcMessage &rpc_msg)
{
	sendRpcValue(rpc_msg.value());
}
/*
chainpack::RpcResponse ServerConnection::sendMessageSync(const chainpack::RpcRequest &rpc_request, int time_out_ms)
{
	Q_UNUSED(rpc_request)
	Q_UNUSED(time_out_ms)
	SHV_EXCEPTION("Sync mesage are nott supported by server connection!");
	return chainpack::RpcResponse();
}
*/
void ServerConnection::onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	//shvInfo() << __FILE__ << RCV_LOG_ARROW << md.toStdString() << shv::chainpack::Utils::toHexElided(data, start_pos, 100);
	if(isInitPhase()) {
		shv::chainpack::RpcValue rpc_val = decodeData(protocol_type, data, start_pos);
		rpc_val.setMetaData(std::move(md));
		cp::RpcMessage msg(rpc_val);
		//logRpcMsg() << msg.toCpon();
		processInitPhase(msg);
		return;
	}
	Super::onRpcDataReceived(protocol_type, std::move(md), data, start_pos, data_len);
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
			shvInfo().nospace() << "Client hello received from: " << socket()->peerAddress().toString().toStdString() << ':' << socket()->peerPort();
			m_helloReceived = true;
			shvInfo() << "sending hello response:" << connectionName();
			m_pendingAuthNonce = shv::chainpack::Utils::toString(std::rand());
			cp::RpcValue::Map params {
				{"nonce", m_pendingAuthNonce}
			};
			sendResponse(rq.requestId(), params);
			return;
		}
		if(m_helloReceived && !m_loginReceived && rq.method() == shv::chainpack::Rpc::METH_LOGIN) {
			shvInfo() << "Client login received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
			cp::RpcValue::Map params = rq.params().toMap();
			m_connectionOptions = params.value(cp::Rpc::KEY_OPTIONS);
			/*
			{
				const chainpack::RpcValue::Map omap = m_connectionOptions.toMap();
				if(omap.hasKey("broker"))
					m_connectionType = ConnectionType::Broker;
				else if(omap.hasKey("device"))
					m_connectionType = ConnectionType::Device;
				else
					m_connectionType = ConnectionType::Client;
			}
			*/
			cp::RpcValue login_resp = login(rq.params());
			if(!login_resp.isValid())
				SHV_EXCEPTION("Invalid authentication for user: " + m_userName + " at: " + connectionName());
			shvInfo().nospace() << "Client logged in user: " << m_userName << " from: " << peerAddress() << ':' << peerPort();
			sendResponse(rq.requestId(), login_resp);
			m_loginReceived = true;
			return;
		}
	}
	catch(shv::core::Exception &e) {
		sendError(rq.requestId(), cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.message()));
	}
	shvError() << "Initial handshake error! Dropping client connection." << connectionName() << msg.toCpon();
	QTimer::singleShot(100, this, &ServerConnection::abort); // need some time to send error to client
}

chainpack::RpcValue ServerConnection::login(const chainpack::RpcValue &auth_params)
{
	const cp::RpcValue::Map params = auth_params.toMap();
	const cp::RpcValue::Map login = params.value(cp::Rpc::KEY_LOGIN).toMap();

	m_userName = login.value("user").toString();
	shvInfo() << "login - user:" << userName();// << "password:" << password_hash;
	bool password_ok = checkPassword(login);
	if(password_ok) {
		cp::RpcValue::Map login_resp;
		//login_resp[cp::Rpc::KEY_CLIENT_ID] = connectionId();
		return login_resp;
	}
	return cp::RpcValue();
}

static std::string sha1_hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(s.data(), s.length());
	return std::string(hash.result().toHex().constData());
}

bool ServerConnection::checkPassword(const chainpack::RpcValue::Map &login)
{
	std::tuple<std::string, PasswordFormat> pwdt = password(userName());
	std::string pwdsrv = std::get<0>(pwdt);
	PasswordFormat pwdsrvfmt = std::get<1>(pwdt);
	if(pwdsrv.empty()) {
		shvError() << "Invalid user name:" << userName();
		return false;
	}
	LoginType login_type = loginTypeFromString(login.value("type").toString());
	if(login_type == LoginType::Invalid)
		login_type = LoginType::Sha1;

	std::string pwdusr = login.value("password").toString();
	if(login_type == LoginType::Plain) {
		if(pwdsrvfmt == PasswordFormat::Plain)
			return (pwdsrv == pwdusr);
		if(pwdsrvfmt == PasswordFormat::Sha1)
			return pwdsrv == sha1_hex(pwdusr);
	}
	if(login_type == LoginType::Sha1) {
		/// login_type == "SHA1" is default
		if(pwdsrvfmt == PasswordFormat::Plain)
			pwdsrv = sha1_hex(pwdsrv);

		std::string nonce = m_pendingAuthNonce + pwdsrv;
		//shvWarning() << m_pendingAuthNonce << "prd" << nonce;
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
		hash.addData(nonce.data(), nonce.length());
		std::string correct_sha1 = std::string(hash.result().toHex().constData());
		//shvInfo() << nonce_sha1 << "vs" << sha1;
		return (pwdusr == correct_sha1);
	}
	{
		shvError() << "Unsupported login type" << loginTypeToString(login_type) << "for user:" << userName();
		return false;
	}
}

std::string ServerConnection::passwordFormatToString(ServerConnection::PasswordFormat f)
{
	switch(f) {
	case PasswordFormat::Plain: return "PLAIN";
	case PasswordFormat::Sha1: return "SHA1";
	default: return "INVALID";
	}
}

ServerConnection::PasswordFormat ServerConnection::passwordFormatFromString(const std::string &s)
{
	std::string typestr = shv::core::String::toUpper(s);
	if(typestr == passwordFormatToString(PasswordFormat::Plain))
		return PasswordFormat::Plain;
	if(typestr == passwordFormatToString(PasswordFormat::Sha1))
		return PasswordFormat::Sha1;
	return PasswordFormat::Invalid;
}

}}}


