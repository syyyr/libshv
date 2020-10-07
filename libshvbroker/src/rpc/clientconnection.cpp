#include "clientconnection.h"
#include "masterbrokerconnection.h"

#include "../brokerapp.h"

#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/accessgrant.h>
#include <shv/chainpack/rpc.h>
#include <shv/coreqt/log.h>
#include <shv/core/stringview.h>
#include <shv/core/utils/serviceproviderpath.h>
#include <shv/core/utils/shvpath.h>
#include <shv/iotqt/rpc/socket.h>

#include <QCryptographicHash>
#include <QTcpSocket>
#include <QTimer>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSubsResolveD() nCDebug("SubsRes").color(NecroLog::Color::LightGreen)

#define ACCESS_EXCEPTION(msg) SHV_EXCEPTION_V(msg, "Access")

namespace cp = shv::chainpack;

using namespace std;

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

std::string ClientConnection::resolveLocalPath(const shv::core::utils::ServiceProviderPath &spp, iotqt::node::ShvNode **pnd)
{
	BrokerApp *app = BrokerApp::instance();
	string local_path;
	iotqt::node::ShvNode *nd = nullptr;
	if(spp.isServicePath()) {
		if(spp.type() == shv::core::utils::ServiceProviderPath::Type::Relative) {
			std::string mount_point = mountPoint();
			if(mount_point.empty())
				SHV_EXCEPTION("Cannot resolve relative path on unmounted device: " + spp.shvPath());
			nd = app->nodeForService(spp);
			if(nd) {
				/// found on this broker
				core::StringView mp_rest = shv::core::utils::ShvPath::mid(mount_point, 1);
				local_path = spp.makePlainPath(mp_rest);
			}
			else {
				/// forward to master broker
				MasterBrokerConnection *mbconn = BrokerApp::instance()->mainMasterBrokerConnection();
				if(!mbconn)
					SHV_EXCEPTION("Cannot forward relative service provider path, no master broker connection, path: " + spp.shvPath());
				/// if the client is mounted on exported path,
				/// then relative path must be resolved with respect to it
				mount_point = mbconn->localPathToMasterExported(mount_point);
				local_path = spp.makeServicePath(mount_point);
			}
		}
		else if(spp.type() == shv::core::utils::ServiceProviderPath::Type::Absolute) {
			nd = app->nodeForService(spp);
			if(nd) {
				/// found on this broker
				local_path = spp.makePlainPath(string());
			}
			else {
				/// forward to master broker
				MasterBrokerConnection *mbconn = BrokerApp::instance()->mainMasterBrokerConnection();
				if(!mbconn)
					SHV_EXCEPTION("Cannot forward relative service provider path, no master broker connection, path: " + spp.shvPath());
				local_path = spp.makeServicePath(string());
			}
		}
	}
	else {
		local_path = spp.shvPath();
	}
	if(pnd)
		*pnd = nd;
	return local_path;
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

ClientConnection::Subscription ClientConnection::createSubscription(const std::string &shv_path, const std::string &method)
{
	logSubscriptionsD() << "Create client subscription for path:" << shv_path << "method:" << method;
	using ServiceProviderPath = shv::core::utils::ServiceProviderPath;
	ServiceProviderPath spp(shv_path);
	string local_path = shv_path;
	iotqt::node::ShvNode *service_node = nullptr;
	{
		local_path = resolveLocalPath(spp, &service_node);
		logSubscriptionsD() << "\t resolved to local path:" << local_path << "node:" << service_node;
		/// check that client has access rights to subscribed signal
		/**
		if path is service provided than
			if it can be served by this proker (service_node != nullptr)
				if it is absolute
					check access rights for absolute path
				else //relative
					do not check ACL for relative paths, client has always access to it's relative service paths
			else // it cannot be served
				do not check ACL not served paths here, forward this to master broker
		else
			check access rights for shv path
		*/
		string shv_path_to_acl_check;
		if(spp.isServicePath()) {
			if(service_node) {
				if(spp.type() == ServiceProviderPath::Type::Absolute) {
					shv_path_to_acl_check = local_path;
				}
			}
		}
		else {
			shv_path_to_acl_check = local_path;
		}
		if(!shv_path_to_acl_check.empty()) {
			cp::AccessGrant acg = BrokerApp::instance()->accessGrantForRequest(this, shv_path_to_acl_check, method, cp::RpcValue());
			int acc_level = shv::iotqt::node::ShvNode::basicGrantToAccessLevel(acg.toRpcValue());
			if(acc_level < cp::MetaMethod::AccessLevel::Read)
				ACCESS_EXCEPTION("Acces to shv signal '" + shv_path + '/' + method + "()' not granted for user '" + loggedUserName() + "'");
		}
	}
	return Subscription(local_path, shv_path, method);
}

string ClientConnection::toSubscribedPath(const CommonRpcClientHandle::Subscription &subs, const string &signal_path) const
{
	using ShvPath = shv::core::utils::ShvPath;
	//using String = shv::core::String;
	using StringView = shv::core::StringView;
	using ServiceProviderPath = shv::core::utils::ServiceProviderPath;
	bool debug = true;
	ServiceProviderPath spp_signal(signal_path);
	ServiceProviderPath spp_subs(subs.subscribedPath);
	/// path must be retraslated if:
	/// * local path is service relative
	/// * local path is not service one and subcribed part is service one
	if(spp_signal.isRelative()) {
		/// a:/mount_point/b/c/d --> a:/b/c/d
		/// a@xy:/mount_point/b/c/d --> a@xy:/b/c/d
		if(debug && !spp_subs.isRelative())
			shvWarning() << "Relative signal must have relative subscription, signal:" << signal_path << "subscription:" << subs.subscribedPath;
		auto a = StringView(subs.subscribedPath);
		auto b = StringView(signal_path).mid(subs.localPath.size());
		return ShvPath::join(a, b);
	}
	else if(spp_signal.isPlain()) {
		if(spp_subs.isRelative()) {
			/// a/b/c/d --> a:/b/c/d
			// cut service and slash
			auto sv = StringView(signal_path).mid(spp_subs.service().length() + 1);
			// cut mount point rest and slash
			const string mount_point = mountPoint();
			StringView mount_point_rest = ShvPath::mid(mount_point, 1);
			sv = sv.mid(mount_point_rest.length() + 1);
			auto ret = ServiceProviderPath::makePath(spp_subs.type(), spp_subs.service(), sv);
			return ret;
		}
		else if(spp_subs.isAbsolute()) {
			/// a/b/c/d --> a|/b/c/d
			// cut service and slash
			auto sv = StringView(signal_path).mid(spp_subs.service().length() + 1);
			return ServiceProviderPath::makePath(spp_subs.type(), spp_subs.service(), sv);
		}
	}
	return signal_path;
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
	//logSubscriptionsD() << "Try propagating client subscription for local path:" << subs.localPath << "method:" << subs.method;
	const std::string &mount_point = mountPoint();
	if(shv::core::utils::ShvPath(subs.localPath).startsWithPath(mount_point)) {
		std::string slave_path = subs.localPath.substr(mount_point.size());
		if(!slave_path.empty()) {
			if(slave_path[0] != '/')
				return;
			slave_path = slave_path.substr(1);
		}
		logSubscriptionsD() << "Propagating client subscription for local path:" << subs.localPath << "method:" << subs.method << "to slave broker as:" << slave_path;
		callMethodSubscribe(slave_path, subs.method);
		return;
	}
	if(shv::core::utils::ShvPath(mount_point).startsWithPath(subs.localPath)
			&& (mount_point.size() == subs.localPath.size() || mount_point[subs.localPath.size()] == '/')
	) {
		logSubscriptionsD() << "Propagating client subscription for local path:" << subs.localPath << "method:" << subs.method << "to slave broker as: ''";
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
