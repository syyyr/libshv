#include "clientconnection.h"
#include "masterbrokerconnection.h"

#include "../brokerapp.h"

#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/accessgrant.h>
#include <shv/chainpack/rpc.h>
#include <shv/coreqt/log.h>
#include <shv/core/stringview.h>
#include <shv/core/utils/shvpath.h>
#include <shv/iotqt/rpc/socket.h>

#include <QCryptographicHash>
#include <QTcpSocket>
#include <QTimer>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSubsResolveD() nCDebug("SubsRes").color(NecroLog::Color::LightGreen)

namespace cp = shv::chainpack;

namespace shv {
namespace broker {
namespace rpc {

ClientConnection::ClientConnection(shv::iotqt::rpc::Socket *socket, QObject *parent)
	: Super(socket, parent)
{
	connect(this, &ClientConnection::socketConnectedChanged, this, &ClientConnection::onSocketConnectedChanged);
}

ClientConnection::~ClientConnection()
{
	//rpc::ServerConnectionshvWarning() << "destroying" << this;
	//shvWarning() << __FUNCTION__;
}

void ClientConnection::onSocketConnectedChanged(bool is_connected)
{
	if(!is_connected) {
		shvInfo() << "Socket disconnected, deleting connection:" << connectionId();
		unregisterAndDeleteLater();
	}
}

shv::chainpack::RpcValue ClientConnection::tunnelOptions() const
{
	return connectionOptions().value(cp::Rpc::KEY_TUNNEL);
}

shv::chainpack::RpcValue ClientConnection::deviceOptions() const
{
	return connectionOptions().value(cp::Rpc::KEY_DEVICE);
}

shv::chainpack::RpcValue ClientConnection::deviceId() const
{
	return deviceOptions().toMap().value(cp::Rpc::KEY_DEVICE_ID);
}

void ClientConnection::setMountPoint(const std::string &mp)
{
	m_mountPoint = mp;
}

int ClientConnection::idleTime() const
{
	if(!m_idleWatchDogTimer || !m_idleWatchDogTimer->isActive())
		return -1;
	int t = m_idleWatchDogTimer->interval() - m_idleWatchDogTimer->remainingTime();
	if(t < 0)
		t = 0;
	return t;
}

int ClientConnection::idleTimeMax() const
{
	if(!m_idleWatchDogTimer || !m_idleWatchDogTimer->isActive())
		return -1;
	return  m_idleWatchDogTimer->interval();
}

void ClientConnection::setIdleWatchDogTimeOut(int sec)
{
	if(sec == 0) {
		static constexpr int MAX_IDLE_TIME_SEC = 10 * 60 * 60;
		shvInfo() << "connection ID:" << connectionId() << "Cannot switch idle watch dog timeout OFF entirely, the inactive connections can last forever then, setting to max time:" << MAX_IDLE_TIME_SEC/60 << "min";
		sec = MAX_IDLE_TIME_SEC;
	}
	if(!m_idleWatchDogTimer) {
		m_idleWatchDogTimer = new QTimer(this);
		connect(m_idleWatchDogTimer, &QTimer::timeout, [this]() {
			std::string mount_point = mountPoint();
			shvError() << "Connection id:" << connectionId() << "device id:" << deviceId().toCpon() << "mount point:" << mount_point
					   << "was idle for more than" << m_idleWatchDogTimer->interval()/1000 << "sec. It will be aborted.";
			unregisterAndDeleteLater();
		});
	}
	shvInfo() << "connection ID:" << connectionId() << "setting idle watch dog timeout to" << sec << "seconds";
	m_idleWatchDogTimer->start(sec * 1000);
}

void ClientConnection::sendMessage(const shv::chainpack::RpcMessage &rpc_msg)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
				<< rpc_msg.toPrettyString();
	Super::sendMessage(rpc_msg);
}

void ClientConnection::sendRawData(const shv::chainpack::RpcValue::MetaData &meta_data, std::string &&data)
{
	logRpcMsg() << SND_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocolType() << shv::chainpack::Rpc::protocolTypeToString(protocolType())
				<< RpcDriver::dataToPrettyCpon(shv::chainpack::RpcMessage::protocolType(meta_data), meta_data, data);
	Super::sendRawData(meta_data, std::move(data));
}

unsigned ClientConnection::addSubscription(const std::string &rel_path, const std::string &method)
{
	Subscription subs = Subscription{rel_path, method};
	return CommonRpcClientHandle::addSubscription(subs);
}

bool ClientConnection::removeSubscription(const std::string &rel_path, const std::string &method)
{
	Subscription subs = Subscription{rel_path, method};
	return CommonRpcClientHandle::removeSubscription(subs);
}

void ClientConnection::onRpcDataReceived(shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	logRpcMsg() << RCV_LOG_ARROW
				<< "client id:" << connectionId()
				<< "protocol_type:" << (int)protocol_type << shv::chainpack::Rpc::protocolTypeToString(protocol_type)
				<< RpcDriver::dataToPrettyCpon(protocol_type, md, data, start_pos, data_len);
	try {
		if(isLoginPhase()) {
			Super::onRpcDataReceived(protocol_type, std::move(md), data, start_pos, data_len);
			return;
		}
		if(m_idleWatchDogTimer)
			m_idleWatchDogTimer->start();
		std::string msg_data(data, start_pos, data_len);
		BrokerApp::instance()->onRpcDataReceived(connectionId(), protocol_type, std::move(md), std::move(msg_data));
	}
	catch (std::exception &e) {
		shvError() << e.what();
	}
}
/*
bool ClientBrokerConnection::checkPassword(const chainpack::UserLogin &login)
{
	return BrokerApp::instance()->aclManager()->checkPassword(login, m_userLoginContext);
}
*/
void ClientConnection::processLoginPhase()
{
	const shv::chainpack::RpcValue::Map &opts = connectionOptions();
	//shvWarning() << connectionId() << cp::RpcValue(opts).toCpon();
	auto t = opts.value(cp::Rpc::OPT_IDLE_WD_TIMEOUT, 3 * 60).toInt();
	setIdleWatchDogTimeOut(t);
	if(tunnelOptions().isMap()) {
		std::string secret = tunnelOptions().toMap().value(cp::Rpc::KEY_SECRET).toString();
		cp::UserLoginResult result;
		result.passwordOk = checkTunnelSecret(secret);
		setLoginResult(result);
		return;
	}
	Super::processLoginPhase();
	cp::UserLoginResult result = BrokerApp::instance()->checkLogin(m_userLoginContext);
	setLoginResult(result);
}

void ClientConnection::setLoginResult(const chainpack::UserLoginResult &result)
{
	auto login_result = result;
	login_result.clientId = connectionId();
	Super::setLoginResult(login_result);
	if(login_result.passwordOk) {
		BrokerApp::instance()->onClientLogin(connectionId());
	}
	else {
		// take some time to send error message and close connection
		QTimer::singleShot(1000, this, &ClientConnection::close);
	}
}

bool ClientConnection::checkTunnelSecret(const std::string &s)
{
	return BrokerApp::instance()->checkTunnelSecret(s);
}

void ClientConnection::propagateSubscriptionToSlaveBroker(const CommonRpcClientHandle::Subscription &subs)
{
	if(!isSlaveBrokerConnection())
		return;
	const std::string &mount_point = mountPoint();
	if(shv::core::utils::ShvPath(subs.absolutePath).startsWithPath(mount_point)) {
		std::string slave_path = subs.absolutePath.substr(mount_point.size());
		if(!slave_path.empty() && slave_path[0] == '/')
			slave_path = slave_path.substr(1);
		callMethodSubscribe(slave_path, subs.method);
		return;
	}
	if(shv::core::utils::ShvPath(mount_point).startsWithPath(subs.absolutePath)) {
		callMethodSubscribe(std::string(), subs.method);
		return;
	}
}
/*
int ClientBrokerConnection::callMethodSubscribeMB(const std::string &shv_path, std::string method)
{
	logSubscriptionsD() << "call subscribe for connection id:" << connectionId() << "path:" << shv_path << "method:" << method;
	return callShvMethod( cp::Rpc::DIR_BROKER_APP
						 , cp::Rpc::METH_SUBSCRIBE
						 , cp::RpcValue::Map{
							 {cp::Rpc::PAR_PATH, shv_path},
							 {cp::Rpc::PAR_METHOD, std::move(method)},
						 }
						 , cp::AccessGrant(cp::Rpc::ROLE_MASTER_BROKER, !cp::AccessGrant::IS_RESOLVED).toRpcValue());
}
*/
}}}
