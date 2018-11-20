#include "iclientconnection.h"

#include <shv/coreqt/log.h>

#include <QCryptographicHash>
#include <QTimer>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

IClientConnection::IClientConnection()
{
}

IClientConnection::~IClientConnection()
{
}

std::string IClientConnection::passwordHash()
{
	if(loginType() == chainpack::AbstractRpcConnection::LoginType::Sha1) {
		if(passwordFormat() == chainpack::AbstractRpcConnection::PasswordFormat::Plain) {
			QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
			std::string pass = password();
			hash.addData(pass.data(), pass.size());
			QByteArray sha1 = hash.result().toHex();
			//shvWarning() << user << pass << sha1;
			return std::string(sha1.constData(), sha1.length());
		}
		if(passwordFormat() == chainpack::AbstractRpcConnection::PasswordFormat::Sha1) {
			return password();
		}
	}
	if(loginType() == chainpack::AbstractRpcConnection::LoginType::Plain) {
		if(passwordFormat() == chainpack::AbstractRpcConnection::PasswordFormat::Plain) {
			return password();
		}
	}
	shvWarning() << "cannot generate password hash for user:" << user();
	return std::string();
}

void IClientConnection::onSocketConnectedChanged(bool is_connected)
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

chainpack::RpcValue IClientConnection::createLoginParams(const chainpack::RpcValue &server_hello)
{
	std::string pass;
	if(loginType() == chainpack::AbstractRpcConnection::LoginType::Sha1) {
		std::string server_nonce = server_hello.toMap().value("nonce").toString();
		std::string pn = server_nonce + passwordHash();
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
		hash.addData(pn.data(), pn.length());
		QByteArray sha1 = hash.result().toHex();
		pass = std::string(sha1.constData(), sha1.size());
	}
	else if(loginType() == chainpack::AbstractRpcConnection::LoginType::Plain) {
		pass = password();
	}
	else {
		shvError() << "Login type:" << chainpack::AbstractRpcConnection::loginTypeToString(loginType()) << "not supported";
	}
	//shvWarning() << password << sha1;
	return cp::RpcValue::Map {
		{"login", cp::RpcValue::Map {
			{"user", user()},
			{"password", pass},
			//{"passwordFormat", chainpack::AbstractRpcConnection::passwordFormatToString(password_format)},
			{"loginType", chainpack::AbstractRpcConnection::loginTypeToString(loginType())},
		 },
		},
		{"options", connectionOptions()},
	};
}

void IClientConnection::processInitPhase(const chainpack::RpcMessage &msg)
{
	do {
		if(!msg.isResponse())
			break;
		cp::RpcResponse resp(msg);
		//shvInfo() << "Handshake response received:" << resp.toCpon();
		if(resp.isError())
			break;
		int id = resp.requestId().toInt();
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

int IClientConnection::brokerClientId() const
{
	return loginResult().value(cp::Rpc::KEY_CLIENT_ID).toInt();
}
/*
std::string ClientConnection::brokerMountPoint() const
{
	return loginResult().value(cp::Rpc::KEY_MOUT_POINT).toString();
}
*/

} // namespace rpc
} // namespace iotqt
} // namespace shv
