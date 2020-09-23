#include "brokerapp.h"
#include "aclmanagersqlite.h"
#include "currentclientshvnode.h"
#include "clientshvnode.h"
#include "brokerappnode.h"
#include "subscriptionsnode.h"
#include "brokeraclnode.h"
#include "clientconnectionnode.h"
#include "rpc/brokertcpserver.h"
#include "rpc/brokerclientserverconnection.h"
#include "rpc/slavebrokerclientconnection.h"

#ifdef WITH_SHV_WEBSOCKETS
#include "rpc/websocketserver.h"
#endif

#include <shv/iotqt/utils/network.h>
#include <shv/iotqt/node/shvnode.h>
#include <shv/iotqt/node/shvnodetree.h>
#include <shv/core/utils/shvpath.h>

#include <shv/coreqt/log.h>

#include <shv/core/string.h>
#include <shv/core/utils.h>
#include <shv/core/assert.h>
#include <shv/core/stringview.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/tunnelctl.h>
#include <shv/chainpack/accessgrant.h>

#include <QFile>
#include <QSocketNotifier>
#include <QSqlDatabase>
#include <QTimer>

#include <ctime>
#include <fstream>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#define logTunnelD() nCDebug("Tunnel")

//#define logAccessM() nCMessage("Access")

#define logAclResolveW() nCWarning("AclResolve")
#define logAclResolveM() nCMessage("AclResolve")

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)
#define logSigResolveD() nCDebug("SigRes").color(NecroLog::Color::LightGreen)

#define ACCESS_EXCEPTION(msg) SHV_EXCEPTION_V(msg, "Access")

namespace cp = shv::chainpack;

using namespace std;

namespace shv {
namespace broker {

#ifdef Q_OS_UNIX
int BrokerApp::m_sigTermFd[2];
#endif

static string CURRENT_CLIENT_SHV_PATH = string(cp::Rpc::DIR_BROKER) + '/' + CurrentClientShvNode::NodeId;

class ClientsNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	ClientsNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods {
			{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
			{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
		}
	{ }

	StringList childNames(const StringViewList &shv_path) override
	{
		auto sl = Super::childNames(shv_path);
		if(shv_path.empty()) {
			std::vector<int> ids;
			ids.reserve(sl.size());
			for(const auto &s : sl)
				ids.push_back(std::stoi(s));
			std::sort(ids.begin(), ids.end());
			for (size_t i = 0; i < ids.size(); ++i)
				sl[i] = std::to_string(ids[i]);
		}
		return sl;
	}
private:
	std::vector<cp::MetaMethod> m_metaMethods;
};

class MasterBrokersNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	MasterBrokersNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods{
				  {cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
				  {cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_CONFIG},
		}
	{ }
private:
	std::vector<cp::MetaMethod> m_metaMethods;
};

class MountsNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	MountsNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(parent)
	{ }

	size_t methodCount(const StringViewList &shv_path) override
	{
		if(shv_path.empty())
			return m_metaMethods.size() - 1;
		if(shv_path.size() == 1)
			return m_metaMethods.size();
		return Super::methodCount(shv_path);
	}

	const shv::chainpack::MetaMethod *metaMethod(const StringViewList &shv_path, size_t ix) override
	{
		if(methodCount(shv_path) <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(methodCount(shv_path)));
		if(shv_path.empty())
			return &(m_metaMethods[ix]);
		if(shv_path.size() == 1)
			return &(m_metaMethods[ix]);
		return Super::metaMethod(shv_path, ix);
	}

	StringList childNames(const StringViewList &shv_path) override
	{
		if(shv_path.empty()) {
			BrokerApp *app = BrokerApp::instance();
			StringList lst;
			for(int id : app->clientConnectionIds()) {
				rpc::BrokerClientServerConnection *conn = app->clientConnectionById(id);
				for(const std::string &mp : conn->mountPoints()) {
					lst.push_back(shv::core::utils::ShvPath::SHV_PATH_QUOTE + mp + shv::core::utils::ShvPath::SHV_PATH_QUOTE);
				}
			}
			std::sort(lst.begin(), lst.end());
			return lst;
		}
		else {
			if(shv_path.size() == 1) {
				return StringList();
			}
		}
		return Super::childNames(shv_path);
	}

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override
	{
		if(shv_path.size() == 1) {
			if(method == METH_CLIENT_IDS) {
				BrokerApp *app = BrokerApp::instance();
				std::string path = shv_path.at(0).slice(1, -1).toString();
				auto *nd1 = app->m_nodesTree->cd(path);
				if(nd1 == nullptr)
					SHV_EXCEPTION("Cannot find node on path: " + path);
				auto *nd = qobject_cast<ClientShvNode*>(nd1);
				if(nd == nullptr)
					SHV_EXCEPTION("Wrong node type on path: " + path + ", looking for ClientShvNode, found: " + nd1->metaObject()->className());
				cp::RpcValue::List lst;
				for(rpc::BrokerClientServerConnection *conn : nd->connections())
					lst.push_back(conn->connectionId());
				return cp::RpcValue{lst};
			}
		}
		return Super::callMethod(shv_path, method, params);
	}
private:
	static const char *METH_CLIENT_IDS;
	static std::vector<cp::MetaMethod> m_metaMethods;
};

const char *MountsNode::METH_CLIENT_IDS = "clientIds";

std::vector<cp::MetaMethod> MountsNode::m_metaMethods = {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_CONFIG},
	{METH_CLIENT_IDS, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_CONFIG},
};

static const auto SQL_CONFIG_CONN_NAME = QStringLiteral("ShvBrokerDbConfigSqlConnection");
//static constexpr int SQL_RECONNECT_INTERVAL = 3000;
BrokerApp::BrokerApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	//shvInfo() << "creating SHV BROKER application object ver." << versionString();
	std::srand(std::time(nullptr));
#ifdef Q_OS_UNIX
	//syslog (LOG_INFO, "Server started");
	installUnixSignalHandlers();
#endif
	m_nodesTree = new shv::iotqt::node::ShvNodeTree(this);
	connect(m_nodesTree->root(), &shv::iotqt::node::ShvRootNode::sendRpcMessage, this, &BrokerApp::onRootNodeSendRpcMesage);
	BrokerAppNode *bn = new BrokerAppNode();
	m_nodesTree->mount(cp::Rpc::DIR_BROKER_APP, bn);
	m_nodesTree->mount(CURRENT_CLIENT_SHV_PATH, new CurrentClientShvNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/clients", new ClientsNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/masters", new MasterBrokersNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/mounts", new MountsNode());
	m_nodesTree->mount(std::string(cp::Rpc::DIR_BROKER) + "/etc/acl", new EtcAclRootNode());

	QTimer::singleShot(0, this, &BrokerApp::lazyInit);
}

BrokerApp::~BrokerApp()
{
	shvInfo() << "Destroying SHV BROKER application object";
}

void BrokerApp::registerLogTopics()
{
	NecroLog::registerTopic("Tunnel", "tunneling");
	NecroLog::registerTopic("Access", "users access log");
	NecroLog::registerTopic("AclResolve", "users and grants resolving");
	NecroLog::registerTopic("AclManager", "ACL manager");
	NecroLog::registerTopic("Subscr", "subscriptions creation and propagation");
	NecroLog::registerTopic("SigRes", "signal resolution in client subscriptions");
	NecroLog::registerTopic("RpcMsg", "dump RPC messages");
	NecroLog::registerTopic("RpcRawMsg", "dump raw RPC messages");
	NecroLog::registerTopic("RpcData", "dump RPC mesages as HEX data");
}

#ifdef Q_OS_UNIX
void BrokerApp::installUnixSignalHandlers()
{
	shvInfo() << "installing Unix signals handlers";
	for(int sig_num : {SIGTERM, SIGHUP}) {
		struct sigaction sa;

		sa.sa_handler = BrokerApp::nativeSigHandler;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags |= SA_RESTART;

		if(sigaction(sig_num, &sa, nullptr) > 0)
			qFatal("Couldn't register posix signal handler");
	}
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigTermFd))
		qFatal("Couldn't create SIG_TERM socketpair");
	m_snTerm = new QSocketNotifier(m_sigTermFd[1], QSocketNotifier::Read, this);
	connect(m_snTerm, &QSocketNotifier::activated, this, &BrokerApp::handlePosixSignals);
	shvInfo() << "SIG_TERM handler installed OK";
}

void BrokerApp::nativeSigHandler(int sig_number)
{
	shvInfo() << "SIG:" << sig_number;
	unsigned char a = static_cast<unsigned char>(sig_number);
	::write(m_sigTermFd[0], &a, sizeof(a));
}

void BrokerApp::handlePosixSignals()
{
	m_snTerm->setEnabled(false);
	unsigned char sig_num;
	::read(m_sigTermFd[1], &sig_num, sizeof(sig_num));

	shvInfo() << "SIG" << sig_num << "catched.";
	if(sig_num == SIGTERM) {
		QTimer::singleShot(0, this, &BrokerApp::quit);
	}
	else if(sig_num == SIGHUP) {
		//QMetaObject::invokeMethod(this, &BrokerApp::reloadConfig, Qt::QueuedConnection); since Qt 5.10
		QTimer::singleShot(0, this, &BrokerApp::reloadConfigRemountDevices);
	}

	m_snTerm->setEnabled(true);
}
#endif

rpc::BrokerTcpServer *BrokerApp::tcpServer()
{
	if(!m_tcpServer)
		SHV_EXCEPTION("TCP server is NULL!");
	return m_tcpServer;
}

rpc::BrokerClientServerConnection *BrokerApp::clientById(int client_id)
{
	return clientConnectionById(client_id);
}

#ifdef WITH_SHV_WEBSOCKETS
rpc::WebSocketServer *BrokerApp::webSocketServer()
{
	if(!m_webSocketServer)
		SHV_EXCEPTION("WebSocket server is NULL!");
	return m_webSocketServer;
}
#endif

void BrokerApp::reloadConfig()
{
	shvInfo() << "Reloading config";
	reloadAcl();
}

void BrokerApp::reloadConfigRemountDevices()
{
	reloadConfig();
	remountDevices();
	//emit configReloaded();
}

void BrokerApp::reloadAcl()
{
	aclManager()->reload();
}

void BrokerApp::startTcpServer()
{
	SHV_SAFE_DELETE(m_tcpServer);
	m_tcpServer = new rpc::BrokerTcpServer(this);
	if(!m_tcpServer->start(cliOptions()->serverPort())) {
		SHV_EXCEPTION("Cannot start TCP server!");
	}
}

void BrokerApp::startWebSocketServers()
{
#ifdef WITH_SHV_WEBSOCKETS
	auto *opts = cliOptions();
	if(opts->serverWebsocketPort_isset()) {
		SHV_SAFE_DELETE(m_webSocketServer);
		int port = opts->serverWebsocketPort();
		if(port > 0) {
		m_webSocketServer = new rpc::WebSocketServer(QWebSocketServer::NonSecureMode, this);
			if(!m_webSocketServer->start(port)) {
				SHV_EXCEPTION("Cannot start WebSocket server!");
			}
		}
	}
	else {
		shvInfo() << "Websocket server port is not set, it will not be started.";
	}
	if(opts->serverWebsocketSslPort_isset()) {
		SHV_SAFE_DELETE(m_webSocketSslServer);
		int port = opts->serverWebsocketSslPort();
		if(port > 0) {
		m_webSocketSslServer = new rpc::WebSocketServer(QWebSocketServer::SecureMode, this);
			if(!m_webSocketSslServer->start(port)) {
				SHV_EXCEPTION("Cannot start WebSocket SSL server!");
			}
		}
	}
	else {
		shvInfo() << "Websocket SSL server port is not set, it will not be started.";
	}
#else
	shvWarning() << "Websocket server is not included in this build";
#endif
}

rpc::BrokerClientServerConnection *BrokerApp::clientConnectionById(int connection_id)
{
	shvLogFuncFrame() << "conn id:" << connection_id;
	rpc::BrokerClientServerConnection *conn = tcpServer()->connectionById(connection_id);
	shvDebug() << "tcp connection:" << conn;
#ifdef WITH_SHV_WEBSOCKETS
	if(!conn && m_webSocketServer) {
		conn = m_webSocketServer->connectionById(connection_id);
		shvDebug() << "ws connection:" << conn;
	}
	if(!conn && m_webSocketSslServer) {
		conn = m_webSocketSslServer->connectionById(connection_id);
		shvDebug() << "wss connection:" << conn;
	}
#endif
	return conn;
}

std::vector<int> BrokerApp::clientConnectionIds()
{
	std::vector<int> ids;
	if(m_tcpServer) {
		ids = m_tcpServer->connectionIds();
	}
#ifdef WITH_SHV_WEBSOCKETS
	if(m_webSocketServer) {
		std::vector<int> ids2 = m_webSocketServer->connectionIds();
		ids.insert( ids.end(), ids2.begin(), ids2.end() );
	}
	if(m_webSocketSslServer) {
		std::vector<int> ids2 = m_webSocketSslServer->connectionIds();
		ids.insert( ids.end(), ids2.begin(), ids2.end() );
	}
#endif
	return ids;
}

void BrokerApp::lazyInit()
{
	initDbConfigSqlConnection();
	reloadConfig();
	startTcpServer();
	startWebSocketServers();
	createMasterBrokerConnections();
}

bool BrokerApp::checkTunnelSecret(const std::string &s)
{
	return m_tunnelSecretList.checkSecret(s);
}

chainpack::UserLoginResult BrokerApp::checkLogin(const chainpack::UserLoginContext &ctx)
{
	return aclManager()->checkPassword(ctx);
}

void BrokerApp::sendNewLogEntryNotify(const std::string &msg)
{
	sendNotifyToSubscribers(".broker/app/log", cp::Rpc::SIG_VAL_CHANGED, msg);
}

void BrokerApp::initDbConfigSqlConnection()
{
	AppCliOptions *opts = cliOptions();
	if(!opts->isSqlConfigEnabled())
		return;
	if(opts->sqlConfigDriver() == "QSQLITE") {
		std::string fn = opts->sqlConfigDatabase();
		if(fn.empty())
			SHV_EXCEPTION("SQL config database not set.");
		if(fn[0] != '/')
			fn = opts->configDir() + '/' + fn;
		QString qfn = QString::fromStdString(fn);
		shvInfo() << "Openning SQL config database:" << fn;
		QSqlDatabase db = QSqlDatabase::addDatabase(QString::fromStdString(cliOptions()->sqlConfigDriver()), SQL_CONFIG_CONN_NAME);
		db.setDatabaseName(qfn);
		if(!db.open())
			SHV_EXCEPTION("Cannot open SQLite SQL config database " + fn);
	}
	else {
		SHV_EXCEPTION("SQL config SQL driver " + opts->sqlConfigDriver() + " is not supported.");
	}
}

AclManager *BrokerApp::createAclManager()
{
	auto *opts = cliOptions();
	if(opts->isSqlConfigEnabled()) {
		return new AclManagerSqlite(this);
	}
	return new AclManagerConfigFiles(this);
}

void BrokerApp::remountDevices()
{
	shvInfo() << "Remounting devices by dropping their connection";
	for(int conn_id : clientConnectionIds()) {
		rpc::BrokerClientServerConnection *conn = clientConnectionById(conn_id);
		if(conn && !conn->mountPoints().empty()) {
			shvInfo() << "Dropping connection ID:" << conn_id << "mounts:" << shv::core::String::join(conn->mountPoints(), ' ');
			conn->close();
		}
	}
}

std::string BrokerApp::resolveMountPoint(const shv::chainpack::RpcValue::Map &device_opts)
{
	std::string mount_point;
	shv::chainpack::RpcValue device_id = device_opts.value(cp::Rpc::KEY_DEVICE_ID);
	if(device_id.isValid())
		mount_point = aclManager()->mountPointForDevice(device_id);
	if(mount_point.empty()) {
		mount_point = device_opts.value(cp::Rpc::KEY_MOUT_POINT).toString();
		std::vector<shv::core::StringView> path = shv::core::utils::ShvPath::split(mount_point);
		if(path.size() && !(path[0] == "test")) {
			shvWarning() << "Mount point can be explicitly specified to test/ dir only, dev id:" << device_id.toCpon();
			mount_point.clear();
		}
	}
	if(mount_point.empty()) {
		shvWarning() << "cannot find mount point for device id:" << device_id.toCpon();// << "connection id:" << connection_id;
	}
	return mount_point;
}

std::string BrokerApp::primaryIPAddress(bool &is_public)
{
	if(cliOptions()->publicIP_isset())
		return cliOptions()->publicIP();
	QHostAddress ha = shv::iotqt::utils::Network::primaryPublicIPv4Address();
	if(!ha.isNull()) {
		is_public = true;
		return ha.toString().toStdString();
	}
	is_public = false;
	ha = shv::iotqt::utils::Network::primaryIPv4Address();
	if(!ha.isNull())
		return ha.toString().toStdString();
	return std::string();
}
/*
void BrokerApp::propagateSubscriptionsToMasterBroker()
{
	logSubscriptionsD() << "Connected to main master broker, propagating client subscriptions.";
	rpc::SlaveBrokerClientConnection *mbrconn = mainMasterBrokerConnection();
	if(!mbrconn)
		return;
	for(int id : clientConnectionIds()) {
		rpc::BrokerClientServerConnection *conn = clientConnectionById(id);
		for (size_t i = 0; i < conn->subscriptionCount(); ++i) {
			const rpc::CommonRpcClientHandle::Subscription &subs = conn->subscriptionAt(i);
			if(shv::core::utils::ShvPath::isRelativePath(subs.absolutePath)) {
				logSubscriptionsD() << "client id:" << id << "propagating subscription for path:" << subs.absolutePath << "method:" << subs.method;
				mbrconn->callMethodSubscribe(subs.absolutePath, subs.method);
			}
		}
	}
}
*/
static bool is_first_path_more_specific(const std::string &path1, const std::string &meth1, const std::string &path2, const std::string &meth2)
{
	if(!meth1.empty() && meth2.empty()) {
		// path with method is always more specific
		return true;
	}
	if(meth1.empty() && !meth2.empty()) {
		return false;
	}
	// naive implementation consider longer path to be more specific
	return path1.size() > path2.size();
}

chainpack::AccessGrant BrokerApp::accessGrantForRequest(rpc::CommonRpcClientHandle *conn, const std::string &rq_shv_path, const std::string &method, const shv::chainpack::RpcValue &rq_grant)
{
	logAclResolveM() << "===== accessGrantForShvPath user:" << conn->loggedUserName() << "requested path:" << rq_shv_path << "method:" << method << "request grant:" << rq_grant.toCpon();
#ifdef USE_SHV_PATHS_GRANTS_CACHE
	PathGrantCache *user_path_grants = m_userPathGrantCache.object(user_name);
	if(user_path_grants) {
		cp::Rpc::AccessGrant *pg = user_path_grants->object(shv_path);
		if(pg) {
			logAclD() << "\t cache hit:" << pg->grant << "weight:" << pg->weight;
			return *pg;
		}
	}
#endif
	bool is_request_from_master_broker = conn->isMasterBrokerConnection();
	auto request_grant = cp::AccessGrant::fromRpcValue(rq_grant);
	if(is_request_from_master_broker) {
		if(request_grant.isValid()) {
			// access resolved by master broker already, forward use this
			return request_grant;
		}
	}
	else {
		if(request_grant.isValid()) {
			logAclResolveM() << "Client defined grants in RPC request are not implemented yet and will be ignored.";
		}
	}
	std::vector<AclManager::FlattenRole> user_roles;
	if(is_request_from_master_broker) {
		// set masterBroker role to requests from master broker without access grant specified
		// This is used mainly for service calls as (un)subscribe propagation to slave brokers etc.
		user_roles = aclManager()->flattenRole(cp::Rpc::ROLE_MASTER_BROKER);
	}
	else {
		user_roles = aclManager()->userFlattenRoles(conn->loggedUserName());
	}
	logAclResolveM() << "roles:";
	for(const AclManager::FlattenRole &flatten_role : user_roles) {
		logAclResolveM() << "\t" << flatten_role.name << " : with weight:" << flatten_role.weight << "nest level:" << flatten_role.nestLevel;
	}
	// find most specific path grant for role with highest weight
	// user_flattent_grants are sorted by weight DESC, nest_leval ASC
	AclAccessRule most_specific_rule;
	if(rq_shv_path == CURRENT_CLIENT_SHV_PATH) {
		// client has WR grant on currentClient node
		most_specific_rule.grant = cp::AccessGrant{cp::Rpc::ROLE_WRITE};
	}
	else {
		int most_specific_role_weight = std::numeric_limits<int>::min();
		shv::iotqt::node::ShvNode::StringViewList shv_path_lst = shv::core::utils::ShvPath::split(rq_shv_path);

		// roles are sorted in weight DESC nest_level ASC
		for(const AclManager::FlattenRole &flatten_role : user_roles) {
			if(most_specific_role_weight != std::numeric_limits<int>::min() && flatten_role.weight < most_specific_role_weight) {
				// roles with lower weight have lower priority, skip them
				break;
			}
			logAclResolveM() << "----- checking role:" << flatten_role.name << "with weight:" << flatten_role.weight << "nest level:" << flatten_role.nestLevel;
			const AclRoleAccessRules &role_rules = aclManager()->accessRoleRules(flatten_role.name);
			if(role_rules.empty()) {
				logAclResolveM() << "\t no paths defined.";
			}
			else for(const AclAccessRule &access_rule : role_rules) {
				//const std::string &role_path = kv.first;

				//const AclAccessRule &acces_grant = kv.second;
				logAclResolveM().nospace() << "\t path: '" << access_rule.pathPattern << "' len: " << access_rule.pathPattern.size();

				shv::iotqt::node::ShvNode::StringViewList rule_path_pattern_list = shv::core::utils::ShvPath::split(access_rule.pathPattern);
				//std::string role_path_method = acces_grant.method;
				if((access_rule.method.empty() || access_rule.method == method)
						&& shv::core::utils::ShvPath::matchWild(shv_path_lst, rule_path_pattern_list)) {
					logAclResolveM().nospace() << "\t\t MATCH, path: '" << access_rule.pathPattern << shv::core::utils::ShvPath::SHV_PATH_METHOD_DELIM << access_rule.method << "'"
											   << " matches request path: '" << rq_shv_path << shv::core::utils::ShvPath::SHV_PATH_METHOD_DELIM << method << "'";
					if(is_first_path_more_specific(access_rule.pathPattern, access_rule.method
												   , most_specific_rule.pathPattern, most_specific_rule.method)) {
						logAclResolveM().nospace() << "\t\t HIT, more specific path found ': " << access_rule.pathPattern << "' grant: " << access_rule.grant.toRpcValue().toCpon();
						most_specific_rule = access_rule;
						//most_specific_path = role_path;
						//most_specific_path_method = role_path_method;
						//most_specific_path_grant = acces_grant.accessGrant;
						most_specific_role_weight = flatten_role.weight;
					}
				}
				else {
					logAclResolveM().nospace() << "\t\t path: '" << access_rule.pathPattern << "' does not match request path: '" << rq_shv_path << ':' << method << "'";
				}
			}
		}
	}
#ifdef USE_SHV_PATHS_GRANTS_CACHE
	if(!user_path_grants) {
		user_path_grants = new PathGrantCache();
		if(!m_userPathGrantCache.insert(user_name, user_path_grants))
			shvError() << "m_userPathGrantCache::insert() error";
	}
	if(user_path_grants)
		user_path_grants->insert(shv_path, new cp::Rpc::AccessGrant(ret));
#endif
	logAclResolveM() << "access user:" << conn->loggedUserName()
				 << "shv_path:" << rq_shv_path
				 << "rq_grant:" << (rq_grant.isValid()? rq_grant.toCpon(): "<none>")
				 << "===> path:" << most_specific_rule.pathPattern << "method:" << most_specific_rule.method << "grant:" << most_specific_rule.grant.toRpcValue().toCpon();
	return most_specific_rule.grant;
}

void BrokerApp::onClientLogin(int connection_id)
{
	rpc::BrokerClientServerConnection *conn = clientConnectionById(connection_id);
	if(!conn)
		SHV_EXCEPTION("Cannot find connection for ID: " + std::to_string(connection_id));
	//const shv::chainpack::RpcValue::Map &opts = conn->connectionOptions();

	shvInfo() << "Client login connection id:" << connection_id;// << "connection type:" << conn->connectionType();
	{
		std::string dir_mount_point = brokerClientDirPath(connection_id);
		{
			shv::iotqt::node::ShvNode *dir = m_nodesTree->cd(dir_mount_point);
			if(dir) {
				shvError() << "Client dir" << dir_mount_point << "exists already and will be deleted, this should never happen!";
				dir->setParentNode(nullptr);
				delete dir;
			}
		}
		shv::iotqt::node::ShvNode *clients_nd = m_nodesTree->mkdir(std::string(cp::Rpc::DIR_BROKER) + "/clients/");
		if(!clients_nd)
			SHV_EXCEPTION("Cannot create parent for ClientDirNode id: " + std::to_string(connection_id));
		ClientConnectionNode *client_id_node = new ClientConnectionNode(connection_id, clients_nd);
		ClientShvNode *client_app_node = new ClientShvNode("app", conn, client_id_node);
		// delete whole client tree, when client is destroyed
		connect(conn, &rpc::BrokerClientServerConnection::destroyed, client_id_node, &ClientShvNode::deleteLater);

		conn->setParent(client_app_node);
		{
			std::string mount_point = client_id_node->shvPath() + "/subscriptions";
			SubscriptionsNode *nd = new SubscriptionsNode(conn);
			if(!m_nodesTree->mount(mount_point, nd))
				SHV_EXCEPTION("Cannot mount connection subscription list to device tree, connection id: " + std::to_string(connection_id)
							  + " shv path: " + mount_point);
		}
	}

	if(conn->deviceOptions().isMap()) {
		const shv::chainpack::RpcValue::Map &device_opts = conn->deviceOptions().toMap();
		std::string mount_point = resolveMountPoint(device_opts);
		if(!mount_point.empty()) {
			ClientShvNode *cli_nd = qobject_cast<ClientShvNode*>(m_nodesTree->cd(mount_point));
			if(cli_nd) {
				/*
				shvWarning() << "The mount point" << mount_point << "exists already";
				if(cli_nd->connection()->deviceId() == device_id) {
					shvWarning() << "The same device ID will be remounted:" << device_id.toCpon();
					delete cli_nd;
				}
				*/
				cli_nd->addConnection(conn);
			}
			else {
				cli_nd = new ClientShvNode(std::string(), conn);
				if(!m_nodesTree->mount(mount_point, cli_nd))
					SHV_EXCEPTION("Cannot mount connection to device tree, connection id: " + std::to_string(connection_id));
			}
			mount_point = cli_nd->shvPath();
			shvInfo() << "client connection id:" << conn->connectionId() << "device id:" << conn->deviceId().toCpon() << " mounted on:" << mount_point;
			/// overwrite client default mount point
			conn->addMountPoint(mount_point);
			connect(conn, &rpc::BrokerClientServerConnection::destroyed, this, [this, connection_id, mount_point]() {
				shvInfo() << "server connection destroyed";
				this->onClientMountedChanged(connection_id, mount_point, false);
			});
			connect(cli_nd, &ClientShvNode::destroyed, cli_nd->parentNode(), &shv::iotqt::node::ShvNode::deleteIfEmptyWithParents, static_cast<Qt::ConnectionType>(Qt::UniqueConnection | Qt::QueuedConnection));
			QTimer::singleShot(0, [this, connection_id, mount_point]() {
				onClientMountedChanged(connection_id, mount_point, true);
			});
		}
	}
	//shvInfo() << m_deviceTree->dumpTree();
}

void BrokerApp::onConnectedToMasterBrokerChanged(int connection_id, bool is_connected)
{
	rpc::SlaveBrokerClientConnection *conn = masterBrokerConnectionById(connection_id);
	if(!conn) {
		shvError() << "Cannot find master broker connection for ID:" << connection_id;
		return;
	}
	std::string masters_path = std::string(cp::Rpc::DIR_BROKER) + "/" + cp::Rpc::DIR_MASTERS;
	shv::iotqt::node::ShvNode *masters_nd = m_nodesTree->cd(masters_path);
	if(!masters_nd) {
		shvError() << ".broker/masters shv directory does not exist, this should never happen";
		return;
	}
	std::string connection_path = masters_path + '/' + conn->objectName().toStdString();
	shv::iotqt::node::ShvNode *node = m_nodesTree->cd(connection_path);
	if(is_connected) {
		shvInfo() << "Logged-in to master broker, connection id:" << connection_id;
		{
			if(node) {
				shvError() << "Master broker connection dir" << connection_path << "exists already and will be deleted, this should never happen!";
				node->setParentNode(nullptr);
				delete node;
			}
			MasterBrokerShvNode *mbnd = new MasterBrokerShvNode(masters_nd);
			mbnd->setNodeId(conn->objectName().toStdString());
			/*shv::iotqt::node::RpcValueMapNode *config_nd = */
			new shv::iotqt::node::RpcValueMapNode("config", conn->options(), mbnd);
			new SubscriptionsNode(conn, mbnd);
		}
		//if(conn == mainMasterBrokerConnection()) {
		//	/// propagate relative subscriptions
		//	propagateSubscriptionsToMasterBroker();
		//}
	}
	else {
		shvInfo() << "Connection to master broker lost, connection id:" << connection_id;
		if(node) {
			node->setParentNode(nullptr);
			delete node;
		}
	}
}

void BrokerApp::onRpcDataReceived(int connection_id, shv::chainpack::Rpc::ProtocolType protocol_type, cp::RpcValue::MetaData &&meta, std::string &&data)
{
	cp::RpcMessage::setProtocolType(meta, protocol_type);
	if(cp::RpcMessage::isRegisterRevCallerIds(meta))
		cp::RpcMessage::pushRevCallerId(meta, connection_id);
	if(cp::RpcMessage::isRequest(meta)) {
		// prepare response for catch block
		// it cannot be constructed from meta, since meta is moved in the try block
		shv::chainpack::RpcResponse rsp = cp::RpcResponse::forRequest(meta);
		try {
			rpc::BrokerClientServerConnection *client_connection = clientConnectionById(connection_id);
			rpc::SlaveBrokerClientConnection *broker_connection = masterBrokerConnectionById(connection_id);
			rpc::CommonRpcClientHandle *connection_handle = client_connection;
			if(connection_handle == nullptr)
				connection_handle = broker_connection;
			std::string shv_path = cp::RpcMessage::shvPath(meta).toString();
			if(connection_handle) {
				/*
				if(shv::core::utils::ShvPath::isRelativePath(shv_path)) {
					rpc::SlaveBrokerClientConnection *master_broker_conn = mainMasterBrokerConnection();
					if(client_connection) {
						shv_path = client_connection->resolveLocalPath(shv_path);
					}
					if(shv::core::utils::ShvPath::isRelativePath(shv_path)) {
						/// still relative path, it should be forwarded to mater broker
						if(master_broker_conn == nullptr)
							ACCESS_EXCEPTION("Cannot resolve relative path " + cp::RpcMessage::shvPath(meta).toString() + ", there is no master broker to forward the request.");
						cp::RpcMessage::setShvPath(meta, shv_path);
						cp::RpcMessage::pushCallerId(meta, connection_id);
						master_broker_conn->sendRawData(std::move(meta), std::move(data));
						return;
					}
					cp::RpcMessage::setShvPath(meta, shv_path);
				}
				*/
				if(client_connection) {
					// erase grant from client connections
					cp::RpcValue ag = cp::RpcMessage::accessGrant(meta);
					if(ag.isValid() /*&& !ag.isUserLogin()*/) {
						shvWarning() << "Client request with access grant specified not allowed, erasing:" << ag.toPrettyString();
						cp::RpcMessage::setAccessGrant(meta, cp::RpcValue());
					}
				}
				const std::string &method = cp::RpcMessage::method(meta).toString();
				cp::AccessGrant acg = accessGrantForRequest(connection_handle, shv_path, method, cp::RpcMessage::accessGrant(meta));
				if(!acg.isValid())
					ACCESS_EXCEPTION("Acces to shv path '" + shv_path + "' not granted for user '" + connection_handle->loggedUserName() + "'");
				cp::RpcMessage::setAccessGrant(meta, acg.toRpcValue());
				cp::RpcMessage::pushCallerId(meta, connection_id);
				if(m_nodesTree->root()) {
					shvDebug() << "REQUEST conn id:" << connection_id << meta.toPrettyString();
					m_nodesTree->root()->handleRawRpcRequest(std::move(meta), std::move(data));
				}
				else {
					SHV_EXCEPTION("Device tree root node is NULL");
				}
			}
			else {
				SHV_EXCEPTION("Connection ID: " + std::to_string(connection_id) + " doesn't exist.");
			}
		}
		catch (std::exception &e) {
			rpc::BrokerClientServerConnection *conn = clientConnectionById(connection_id);
			if(conn) {
				rsp.setError(cp::RpcResponse::Error::create(
								 cp::RpcResponse::Error::MethodCallException
								 , e.what()));
				conn->sendMessage(rsp);
			}
		}
	}
	else if(cp::RpcMessage::isResponse(meta)) {
		shvDebug() << "RESPONSE conn id:" << connection_id << meta.toPrettyString();
		shv::chainpack::RpcValue orig_caller_ids = cp::RpcMessage::callerIds(meta);
		cp::RpcValue::Int caller_id = cp::RpcMessage::popCallerId(meta);
		shvDebug() << "top caller id:" << caller_id;
		if(caller_id > 0) {
			cp::TunnelCtl tctl = cp::RpcMessage::tunnelCtl(meta);
			if(tctl.state() == cp::TunnelCtl::State::FindTunnelRequest) {
				logTunnelD() << "FindTunnelRequest received:" << meta.toPrettyString();
				cp::FindTunnelReqCtl find_tunnel_request(tctl);
				//uint32_t tctl_ipv4_addr = utils::Network::toIntIPv4Address(find_tunnel_request.host());
				//bool tctl_ipv4_addr_is_public = utils::Network::isPublicIPv4Address(tctl_ipv4_addr);
				bool last_broker = cp::RpcValueGenList(cp::RpcMessage::callerIds(meta)).empty();
				bool is_public_node;
				std::string server_ip = primaryIPAddress(is_public_node);
				if(is_public_node || (last_broker && find_tunnel_request.host().empty())) {
					find_tunnel_request.setHost(server_ip);
					find_tunnel_request.setPort(tcpServer()->serverPort());
					find_tunnel_request.setCallerIds(orig_caller_ids);
					find_tunnel_request.setSecret(m_tunnelSecretList.createSecret());
				}
				if(last_broker) {
					cp::FindTunnelRespCtl find_tunnel_response = cp::FindTunnelRespCtl::fromFindTunnelRequest(find_tunnel_request);
					cp::RpcMessage msg;
					msg.setRequestId(cp::RpcMessage::requestId(meta));
					// send response to FindTunnelRequest, remove top client id, since it is this connection id
					msg.setCallerIds(cp::RpcMessage::revCallerIds(meta));
					int top_connection_id = msg.popCallerId();
					if(top_connection_id != connection_id) {
						shvError() << "(top_connection_id != connection_id) this should never happen";
						return;
					}
					msg.setTunnelCtl(find_tunnel_response);
					rpc::CommonRpcClientHandle *cch = commonClientConnectionById(connection_id);
					if(cch) {
						logTunnelD() << "Sending FindTunnelResponse:" << msg.toPrettyString();
						cch->sendMessage(msg);
					}
					return;
				}
				cp::RpcMessage::setTunnelCtl(meta, find_tunnel_request);
				logTunnelD() << "Forwarding FindTunnelRequest:" << meta.toPrettyString();
			}
			rpc::CommonRpcClientHandle *cch = commonClientConnectionById(caller_id);
			if(cch) {
				cch->sendRawData(std::move(meta), std::move(data));
			}
			else {
				shvWarning() << "Got RPC response for not-exists connection, may be it was closed meanwhile. Connection id:" << caller_id;
			}
		}
		else {
			// broker messages like create master broker subscription
			shvDebug() << "Got RPC response without src connection specified, it should be this broker call like create master broker subscription, throwing message away." << meta.toPrettyString();
		}
	}
	else if(cp::RpcMessage::isSignal(meta)) {
		logSigResolveD() << "SIGNAL:" << meta.toPrettyString() << "from:" << connection_id;

		/// if signal arrives from client, its path must be prepended by client mount points
		/// if signal arrives from master broker, it can happen in case of relative subscription only,
		///   then its path must be left untouched
		std::vector<std::string> mount_points;

		rpc::BrokerClientServerConnection *client_connection = clientConnectionById(connection_id);
		//rpc::MasterBrokerConnection *broker_connection = masterBrokerConnectionById(connection_id);
		//rpc::CommonRpcClientHandle *connection_handle = nullptr;
		if(client_connection) {
			//connection_handle = client_connection;
			mount_points = client_connection->mountPoints();
		}
		else {
			//connection_handle = broker_connection;
			mount_points = {std::string()};
		}
		const std::string sig_shv_path = cp::RpcMessage::shvPath(meta).toString();
		for(const std::string &mp : mount_points) {
			std::string full_shv_path = shv::core::utils::ShvPath::join(mp, sig_shv_path);
			if(full_shv_path.empty()) {
				shvError() << "SIGNAL with empty shv path received from master broker connection.";
			}
			else {
				//logSigResolveD() << client_connection->connectionId() << "forwarding signal to client on mount point:" << mp << "as:" << full_shv_path;
				cp::RpcMessage::setShvPath(meta, full_shv_path);
				bool sig_sent = sendNotifyToSubscribers(meta, data);
				if(!sig_sent && client_connection->isSlaveBrokerConnection()) {
					logSubscriptionsD() << "Rejecting unsubscribed signal, shv_path:" << full_shv_path << "method:" << cp::RpcMessage::method(meta).toString();
					cp::RpcRequest rq;
					rq.setRequestId(0);
					rq.setMethod(cp::Rpc::METH_REJECT_NOT_SUBSCRIBED)
							.setParams(cp::RpcValue::Map{
										   { cp::Rpc::PAR_PATH, sig_shv_path},
										   { cp::Rpc::PAR_METHOD, cp::RpcMessage::method(meta).toString()}})
							.setShvPath(cp::Rpc::DIR_BROKER_APP);
					client_connection->sendMessage(rq);
				}
			}
		}
	}
}

void BrokerApp::onRootNodeSendRpcMesage(const shv::chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		shv::chainpack::RpcValue::Int connection_id = resp.popCallerId();
		rpc::CommonRpcClientHandle *conn = commonClientConnectionById(connection_id);
		if(conn)
			conn->sendMessage(resp);
		else
			shvError() << "Cannot find connection for ID:" << connection_id;
		return;
	}
	else if(msg.isSignal()) {
		cp::RpcSignal sig(msg);
		sendNotifyToSubscribers(sig.shvPath().toString(), sig.method().toString(), sig.params());
	}
	else {
		shvError() << "Send message not implemented.";// << msg.toCpon();
	}
}

void BrokerApp::onClientMountedChanged(int client_id, const std::string &mount_point, bool is_mounted)
{
	sendNotifyToSubscribers(mount_point, cp::Rpc::SIG_MOUNTED_CHANGED, is_mounted);
	if(is_mounted) {
		//sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_CONNECTED, cp::RpcValue());
		rpc::BrokerClientServerConnection *cc = clientConnectionById(client_id);
		if(cc && cc->isSlaveBrokerConnection()) {
			/// if slave broker is connected, forward subscriptions of connected clients
			for(rpc::CommonRpcClientHandle *ch : allClientConnections()) {
				if(ch->connectionId() == client_id)
					continue;
				for(size_t i=0; i<ch->subscriptionCount(); i++) {
					const rpc::CommonRpcClientHandle::Subscription &subs = ch->subscriptionAt(i);
					cc->propagateSubscriptionToSlaveBroker(subs);
				}
			}
		}
	}
	else {
		//this->sendNotifyToSubscribers(connection_id, mount_point, cp::Rpc::NTF_DISCONNECTED, cp::RpcValue());
	}
}

std::string BrokerApp::brokerClientDirPath(int client_id)
{
	return std::string(cp::Rpc::DIR_BROKER) + "/clients/" + std::to_string(client_id);
}

std::string BrokerApp::brokerClientAppPath(int client_id)
{
	return brokerClientDirPath(client_id) + "/app";
}

bool BrokerApp::sendNotifyToSubscribers(const shv::chainpack::RpcValue::MetaData &meta_data, const std::string &data)
{
	// send it to all clients for now
	bool subs_sent = false;
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		if(conn->isConnectedAndLoggedIn()) {
			const cp::RpcValue shv_path = cp::RpcMessage::shvPath(meta_data);
			const cp::RpcValue method = cp::RpcMessage::method(meta_data);
			int subs_ix = conn->isSubscribed(shv_path.toString(), method.toString());
			if(subs_ix >= 0) {
				//shvDebug() << "\t broadcasting to connection id:" << id;
				//const rpc::BrokerClientServerConnection::Subscription &subs = conn->subscriptionAt((size_t)subs_ix);
				std::string new_path = conn->toSubscribedPath(shv_path.toString());
				if(new_path == shv_path.toString()) {
					conn->sendRawData(meta_data, std::string(data));
				}
				else {
					shv::chainpack::RpcValue::MetaData md2(meta_data);
					cp::RpcMessage::setShvPath(md2, new_path);
					conn->sendRawData(md2, std::string(data));
				}
				subs_sent = true;
			}
		}
	}
	return subs_sent;
}

void BrokerApp::sendNotifyToSubscribers(const std::string &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	//shvWarning() << shv_path << method << params.toPrettyString();
	cp::RpcSignal sig;
	sig.setShvPath(shv_path);
	sig.setMethod(method);
	sig.setParams(params);
	// send it to all clients for now
	for(rpc::CommonRpcClientHandle *conn : allClientConnections()) {
		if(conn->isConnectedAndLoggedIn()) {
			int subs_ix = conn->isSubscribed(shv_path, method);
			if(subs_ix >= 0) {
				//shvDebug() << "\t broadcasting to connection id:" << id;
				//const rpc::BrokerClientServerConnection::Subscription &subs = conn->subscriptionAt((size_t)subs_ix);
				std::string new_path = conn->toSubscribedPath(shv_path);
				if(new_path != shv_path)
					sig.setShvPath(new_path);
				conn->sendMessage(sig);
			}
		}
	}
}

void BrokerApp::addSubscription(int client_id, const std::string &shv_path, const std::string &method)
{
	rpc::CommonRpcClientHandle *connection_handle = commonClientConnectionById(client_id);
	if(!connection_handle)
		SHV_EXCEPTION("Connot create subscription, client doesn't exist.");
	//logSubscriptionsD() << "addSubscription connection id:" << client_id << "path:" << path << "method:" << method;
	rpc::BrokerClientServerConnection *cli = clientById(client_id);
	if(cli && !cli->isSlaveBrokerConnection()) {
		// check that client has access rights to subscribed signal
		// do not check slave broker connections - cli->isSlaveBrokerConnection()
		// and requests from super-broker - cli == nullptr
		cp::AccessGrant acg = accessGrantForRequest(connection_handle, shv_path, method, cp::RpcValue());
		int acc_level = shv::iotqt::node::ShvNode::basicGrantToAccessLevel(acg.toRpcValue());
		if(acc_level < cp::MetaMethod::AccessLevel::Read)
			ACCESS_EXCEPTION("Acces to shv signal '" + shv_path + '/' + method + "()' not granted for user '" + connection_handle->loggedUserName() + "'");
	}
	auto subs_ix = connection_handle->addSubscription(shv_path, method);
	const rpc::CommonRpcClientHandle::Subscription &subs = connection_handle->subscriptionAt(subs_ix);
	/*
	if(shv::core::utils::ShvPath::isRelativePath(subs.absolutePath)) {
		/// still relative path, should be propagated to the master broker
		rpc::SlaveBrokerClientConnection *mbrconn = mainMasterBrokerConnection();
		if(mbrconn) {
			mbrconn->callMethodSubscribe(subs.absolutePath, subs.method);
		}
		else {
			shvError() << "Cannot propagate relative path subscription, without master broker connected:" << subs.absolutePath;
		}
	}
	else
	*/
	{
		/// check slave broker connections
		/// whether this subsciption should be propagated to them
		for (int connection_id : clientConnectionIds()) {
			rpc::BrokerClientServerConnection *conn = clientConnectionById(connection_id);
			if(conn->isSlaveBrokerConnection()) {
				conn->propagateSubscriptionToSlaveBroker(subs);
			}
		}
	}
}

bool BrokerApp::removeSubscription(int client_id, const std::string &shv_path, const std::string &method)
{
	rpc::CommonRpcClientHandle *conn = commonClientConnectionById(client_id);
	if(!conn)
		SHV_EXCEPTION("Connot remove subscription, client doesn't exist.");
	//logSubscriptionsD() << "addSubscription connection id:" << client_id << "path:" << path << "method:" << method;
	return conn->removeSubscription(shv_path, method);
}

bool BrokerApp::rejectNotSubscribedSignal(int client_id, const std::string &path, const std::string &method)
{
	logSubscriptionsD() << "signal rejected, shv_path:" << path << "method:" << method;
	rpc::SlaveBrokerClientConnection *conn = masterBrokerConnectionById(client_id);
	if(conn) {
		return conn->rejectNotSubscribedSignal(conn->masterExportedToLocalPath(path), method);
	}
	return false;
}

void BrokerApp::createMasterBrokerConnections()
{
	if(!cliOptions()->isMasterBrokersEnabled())
		return;
	shvInfo() << "Creating master broker connections";
	shv::chainpack::RpcValue masters = cliOptions()->masterBrokersConnections();

	for(const auto &kv : masters.toMap()) {
		cp::RpcValue::Map opts = kv.second.toMap();

		if (cliOptions()->masterBrokerDeviceId_isset()) {
			cp::RpcValue::Map dev = opts.value("device").toMap();
			dev.setValue("id", cliOptions()->masterBrokerDeviceId());
			opts.setValue("device", dev);
		}
		shvInfo() << "master broker device ID:" << opts.value("device").toPrettyString() << "for connection:" << kv.first;

		if(opts.value("enabled").toBool() == false)
			continue;

		shvInfo() << "creating master broker connection:" << kv.first;
		rpc::SlaveBrokerClientConnection *bc = new rpc::SlaveBrokerClientConnection(this);
		bc->setObjectName(QString::fromStdString(kv.first));
		int id = bc->connectionId();
		connect(bc, &rpc::SlaveBrokerClientConnection::brokerConnectedChanged, this, [id, this](bool is_connected) {
			this->onConnectedToMasterBrokerChanged(id, is_connected);
		});
		bc->setOptions(opts);
		bc->open();
	}
}

QList<rpc::SlaveBrokerClientConnection *> BrokerApp::masterBrokerConnections() const
{
	return findChildren<rpc::SlaveBrokerClientConnection*>(QString(), Qt::FindDirectChildrenOnly);
}

rpc::SlaveBrokerClientConnection *BrokerApp::masterBrokerConnectionById(int connection_id)
{
	for(rpc::SlaveBrokerClientConnection *bc : masterBrokerConnections()) {
		if(bc->connectionId() == connection_id)
			return bc;
	}
	return nullptr;
}

std::vector<rpc::CommonRpcClientHandle *> BrokerApp::allClientConnections()
{
	std::vector<rpc::CommonRpcClientHandle *> ret;
	for (int i : clientConnectionIds())
		ret.push_back(clientConnectionById(i));
	QList<rpc::SlaveBrokerClientConnection *> mbc = masterBrokerConnections();
	std::copy(mbc.begin(), mbc.end(), std::back_inserter(ret));
	return ret;
}

rpc::CommonRpcClientHandle *BrokerApp::commonClientConnectionById(int connection_id)
{
	rpc::CommonRpcClientHandle *ret = clientConnectionById(connection_id);
	if(ret)
		return ret;
	ret = masterBrokerConnectionById(connection_id);
	return ret;
}

QSqlDatabase BrokerApp::sqlConfigConnection()
{
	if(!cliOptions()->isSqlConfigEnabled())
		return QSqlDatabase();
	QSqlDatabase db = QSqlDatabase::database(SQL_CONFIG_CONN_NAME, false);
	return db;
}

AclManager *BrokerApp::aclManager()
{
	if(m_aclManager == nullptr)
		setAclManager(createAclManager());
	return m_aclManager;
}

void BrokerApp::setAclManager(AclManager *mng)
{
	m_aclManager = mng;
}


}}
