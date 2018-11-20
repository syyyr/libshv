#ifndef ICLIENTCONNECTION_H
#define ICLIENTCONNECTION_H

#include "../shviotqtglobal.h"

#include <shv/chainpack/abstractrpcconnection.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/core/utils.h>

class QTimer;

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT IClientConnection
{
	SHV_FIELD_IMPL(std::string, u, U, ser)
	SHV_FIELD_IMPL(std::string, h, H, ost)
	SHV_FIELD_IMPL2(int, p, P, ort, shv::chainpack::AbstractRpcConnection::DEFAULT_RPC_BROKER_PORT)
	SHV_FIELD_IMPL(std::string, p, P, assword)
	SHV_FIELD_IMPL(shv::chainpack::AbstractRpcConnection::PasswordFormat, p, P, asswordFormat)
	SHV_FIELD_IMPL(shv::chainpack::AbstractRpcConnection::LoginType, l, L, oginType)

	//SHV_FIELD_IMPL(std::string,c, C, onnectionType) // [device | tunnel | client]
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, c, C, onnectionOptions)

public:
	IClientConnection();
	virtual ~IClientConnection();

	const shv::chainpack::RpcValue::Map& loginResult() const {return m_connectionState.loginResult.toMap();}
	int brokerClientId() const;
protected:
	bool isInitPhase() const {return !isBrokerConnected();}
	virtual void processInitPhase(const chainpack::RpcMessage &msg);
	virtual shv::chainpack::RpcValue createLoginParams(const shv::chainpack::RpcValue &server_hello);

	virtual std::string passwordHash();

	virtual void onSocketConnectedChanged(bool is_connected);

	virtual void sendHello() = 0;
	virtual void sendLogin(const shv::chainpack::RpcValue &server_hello) = 0;
	virtual void setBrokerConnected(bool b) = 0;
	virtual void resetConnection() = 0;
	bool isBrokerConnected() const {return m_connectionState.isBrokerConnected;}
protected:
	struct ConnectionState
	{
		bool isBrokerConnected = false;
		int helloRequestId = 0;
		int loginRequestId = 0;
		int pingRqId = 0;
		shv::chainpack::RpcValue loginResult;
	};

	ConnectionState m_connectionState;
	int m_checkBrokerConnectedInterval = 0;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv

#endif // ICLIENTCONNECTION_H
