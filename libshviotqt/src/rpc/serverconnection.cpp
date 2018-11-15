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

//#define logRpcMsg() shvCDebug("RpcMsg")

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

static int s_initPhaseTimeout = 10000;

ServerConnection::ServerConnection(QTcpSocket *socket, QObject *parent)
	: Super(parent)
{
	setSocket(socket);
	socket->setParent(nullptr);
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
			shvInfo() << "Client hello received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
			//const shv::chainpack::RpcValue::String profile = m.value("profile").toString();
			//m_profile = profile;
			m_helloReceived = true;
			shvInfo() << "sending hello response:" << connectionName();// << "profile:" << m_profile;
			m_pendingAuthNonce = shv::chainpack::Utils::toString(std::rand());
			cp::RpcValue::Map params {
				//{"protocol", cp::RpcValue::Map{{"version", protocol_version}}},
				{"nonce", m_pendingAuthNonce}
			};
			sendResponse(rq.requestId(), params);
			return;
		}
		if(m_helloReceived && !m_loginReceived && rq.method() == shv::chainpack::Rpc::METH_LOGIN) {
			shvInfo() << "Client login received";// << profile;// << "device id::" << m.value("deviceId").toStdString();
			cp::RpcValue::Map params = rq.params().toMap();
			m_connectionOptions = params.value(cp::Rpc::KEY_CONNECTION_OPTIONS);
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
	if(m_userName.empty())
		return false;

	shvInfo() << "login - user:" << m_userName;// << "password:" << password_hash;
	bool password_ok = checkPassword(login);
	if(password_ok) {
		cp::RpcValue::Map login_resp;
		//login_resp[cp::Rpc::KEY_CLIENT_ID] = connectionId();
		return login_resp;
	}
	return cp::RpcValue();
}

bool ServerConnection::checkPassword(const chainpack::RpcValue::Map &login)
{
	bool password_ok = false;
	std::string login_type = login.value("type").toString();
	shv::core::String::upper(login_type);
	if(login_type == "RSA-OAEP") {
		shvError() << "RSA-OAEP" << "login type not supported yet";
		//std::string password_hash = passwordHash(PasswordHashType::RsaOaep, m_user);
	}
	else if(login_type == "PLAIN") {
		std::string password_hash = passwordHash(LoginType::Plain, m_userName);
		std::string pwd = login.value("password").toString();
		password_ok = (pwd == password_hash);
	}
	else {
		/// login_type == "SHA1" is default
		std::string sent_sha1 = login.value("password").toString();

		std::string password_hash = passwordHash(LoginType::Sha1, m_userName);
		std::string nonce = m_pendingAuthNonce + password_hash;
		//shvWarning() << m_pendingAuthNonce << "prd" << nonce;
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
		hash.addData(nonce.data(), nonce.length());
		std::string correct_sha1 = std::string(hash.result().toHex().constData());
		//shvInfo() << nonce_sha1 << "vs" << sha1;
		password_ok = (sent_sha1 == correct_sha1);
	}
	return password_ok;
}

}}}


