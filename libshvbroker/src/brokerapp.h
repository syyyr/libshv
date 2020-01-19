#pragma once

#ifdef USE_SHV_PATHS_GRANTS_CACHE
#include <string>

inline unsigned qHash(const std::string &s) noexcept //Q_DECL_NOEXCEPT_EXPR(noexcept(qHash(s)))
{
	std::hash<std::string> hash_fn;
	return hash_fn(s);
}
#include <QCache>
#endif

#include "shvbrokerglobal.h"
#include "appclioptions.h"
#include "tunnelsecretlist.h"
#include "aclmanager.h"

#include <shv/iotqt/node/shvnode.h>

#include <shv/chainpack/rpcvalue.h>
#include <shv/iotqt/rpc/password.h>

#include <QCoreApplication>
#include <QDateTime>

#include <set>

class QSocketNotifier;

namespace shv { namespace iotqt { namespace node { class ShvNodeTree; }}}
//namespace shv { namespace iotqt { namespace rpc { struct Password; }}}
namespace shv { namespace chainpack { class RpcSignal; }}

namespace shv {
namespace broker {

namespace rpc { class WebSocketServer; class BrokerTcpServer; class ClientBrokerConnection;  class MasterBrokerConnection; class CommonRpcClientHandle; }

class AclManager;

class SHVBROKER_DECL_EXPORT BrokerApp : public QCoreApplication
{
	Q_OBJECT
private:

	using Super = QCoreApplication;
	friend class AclManager;
	friend class MountsNode;
public:
	BrokerApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~BrokerApp() Q_DECL_OVERRIDE;

	virtual AppCliOptions* cliOptions() {return m_cliOptions;}
	static void registerLogTopics();

	static BrokerApp* instance() {return qobject_cast<BrokerApp*>(Super::instance());}

	void onClientLogin(int connection_id);
	void onConnectedToMasterBrokerChanged(int connection_id, bool is_connected);

	rpc::MasterBrokerConnection* mainMasterBrokerConnection() { return masterBrokerConnections().value(0); }

	void onRpcDataReceived(int connection_id, shv::chainpack::Rpc::ProtocolType protocol_type, shv::chainpack::RpcValue::MetaData &&meta, std::string &&data);

	void addSubscription(int client_id, const std::string &path, const std::string &method);
	bool removeSubscription(int client_id, const std::string &shv_path, const std::string &method);
	bool rejectNotSubscribedSignal(int client_id, const std::string &path, const std::string &method);

	rpc::BrokerTcpServer* tcpServer();
	rpc::ClientBrokerConnection* clientById(int client_id);

#ifdef WITH_SHV_WEBSOCKETS
	rpc::WebSocketServer* webSocketServer();
#endif

	rpc::CommonRpcClientHandle* commonClientConnectionById(int connection_id);

	AclManager *aclManager();
	void setAclManager(AclManager *mng);
	void reloadConfig();
	Q_SIGNAL void configReloaded();
	//void clearAccessGrantCache();
	/*
	shv::chainpack::RpcValue fstabConfig() { return aclConfig("fstab", !shv::core::Exception::Throw); }
	shv::chainpack::RpcValue usersConfig() { return aclConfig("users", !shv::core::Exception::Throw); }
	shv::chainpack::RpcValue grantsConfig() { return aclConfig("grants", !shv::core::Exception::Throw); }
	shv::chainpack::RpcValue pathsConfig() { return aclConfig("paths", !shv::core::Exception::Throw); }

	shv::chainpack::RpcValue aclConfig(const std::string &config_name, bool throw_exc);
	bool setAclConfig(const std::string &config_name, const shv::chainpack::RpcValue &config, bool throw_exc);
	*/
	bool checkTunnelSecret(const std::string &s);

	std::string dataToCpon(shv::chainpack::Rpc::ProtocolType protocol_type, const shv::chainpack::RpcValue::MetaData &md, const std::string &data, size_t start_pos = 0, size_t data_len = 0);

private:
	void remountDevices();
	void reloadAcl();

	void lazyInit();

	QString serverProfile(); // unified access via Globals::serverProfile()

	void startTcpServer();

	void startWebSocketServer();

	rpc::ClientBrokerConnection* clientConnectionById(int connection_id);
	std::vector<int> clientConnectionIds();

	void createMasterBrokerConnections();
	QList<rpc::MasterBrokerConnection*> masterBrokerConnections() const;
	rpc::MasterBrokerConnection* masterBrokerConnectionById(int connection_id);

	std::vector<rpc::CommonRpcClientHandle *> allClientConnections();

	std::string resolveMountPoint(const shv::chainpack::RpcValue::Map &device_opts);

	chainpack::AccessGrant accessGrantForRequest(rpc::CommonRpcClientHandle *conn, const std::string &rq_shv_path, const chainpack::RpcValue &rq_grant);

	void onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg);

	void onClientMountedChanged(int client_id, const std::string &mount_point, bool is_mounted);

	void sendNotifyToSubscribers(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params);
	bool sendNotifyToSubscribers(const shv::chainpack::RpcValue::MetaData &meta_data, const std::string &data);

	static std::string brokerClientDirPath(int client_id);
	static std::string brokerClientAppPath(int client_id);

	std::string primaryIPAddress(bool &is_public);

	void propagateSubscriptionsToMasterBroker();
protected:
	AppCliOptions *m_cliOptions;
	rpc::BrokerTcpServer *m_tcpServer = nullptr;
#ifdef WITH_SHV_WEBSOCKETS
	rpc::WebSocketServer *m_webSocketServer = nullptr;
#endif
	shv::iotqt::node::ShvNodeTree *m_nodesTree = nullptr;
	TunnelSecretList m_tunnelSecretList;
#ifdef USE_SHV_PATHS_GRANTS_CACHE
	using PathGrantCache = QCache<std::string, shv::chainpack::Rpc::AccessGrant>;
	using UserPathGrantCache = QCache<std::string, PathGrantCache>;
	UserPathGrantCache m_userPathGrantCache;
#endif
	AclManager *m_aclManager = nullptr;
#ifdef Q_OS_UNIX
private:
	// Unix signal handlers.
	// You can't call Qt functions from Unix signal handlers,
	// but you can write to socket
	void installUnixSignalHandlers();
	Q_SLOT void handlePosixSignals();
	static void nativeSigHandler(int sig_number);

	static int m_sigTermFd[2];
	QSocketNotifier *m_snTerm = nullptr;
#endif
};

}}