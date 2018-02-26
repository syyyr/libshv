#pragma once

#include "../shviotqtglobal.h"
#include "../chainpack/rpcconnection.h"

#include <shv/core/utils.h>
#include <shv/coreqt/utils.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpcdriver.h>

#include <QAbstractSocket>
#include <QObject>

namespace shv { namespace chainpack { class RpcMessage; }}

namespace shv {
namespace iotqt {
namespace client {

class SHVIOTQT_DECL_EXPORT ClientConnection : public shv::iotqt::chainpack::RpcConnection
{
	Q_OBJECT
	using Super = shv::iotqt::chainpack::RpcConnection;

	SHV_FIELD_IMPL(std::string, u, U, ser)
	//SHV_FIELD_IMPL(std::string, p, P, rofile)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, d, D, evice)

	SHV_PROPERTY_BOOL_IMPL(b, B, rokerConnected)
public:
	explicit ClientConnection(QObject *parent = 0);
	~ClientConnection() Q_DECL_OVERRIDE;
protected:
	bool onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) override;
private:
	std::string passwordHash(const std::string &user);
	void onSocketConnectedChanged(bool is_connected);
	void sendHello();
	void sendLogin(const shv::chainpack::RpcValue &server_hello);
private:
	//void lublicatorTesting();
private:
	unsigned m_helloRequestId = 0;
	unsigned m_loginRequestId = 0;
};

}}}

