#include "brokerappnode.h"

#include "brokerapp.h"
#include "rpc/clientconnectiononbroker.h"
#include "rpc/masterbrokerconnection.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcmessage.h>
//#include <shv/chainpack/rpcdriver.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>
#include <shv/core/log.h>
#include <shv/iotqt/rpc/rpcresponsecallback.h>

#include <QTimer>

#define logSubscriptionsD() nCDebug("Subscr").color(NecroLog::Color::Yellow)

namespace cp = shv::chainpack;

namespace shv {
namespace broker {

namespace {
static const char M_GET_VERBOSITY[] = "verbosity";
static const char M_SET_VERBOSITY[] = "setVerbosity";
static const char M_GET_ENABLED[] = "enabled";
static const char M_SET_ENABLED[] = "setEnabled";
class BrokerLogNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	BrokerLogNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super("log", &m_metaMethods, parent)
		, m_metaMethods {
			{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
			{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
			{cp::Rpc::SIG_VAL_CHANGED, cp::MetaMethod::Signature::VoidParam, cp::MetaMethod::Flag::IsSignal, cp::Rpc::ROLE_READ},
			{M_GET_ENABLED, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
			{M_SET_ENABLED, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::IsSetter, cp::Rpc::ROLE_WRITE},
			{M_GET_VERBOSITY, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
			{M_SET_VERBOSITY, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::IsSetter, cp::Rpc::ROLE_COMMAND},
		}
	{ }

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override
	{
		if(shv_path.empty()) {
			if(method == M_GET_ENABLED) {
				return BrokerApp::instance()->isLogEntryNotyfyEnabled();
			}
			if(method == M_SET_ENABLED) {
				BrokerApp::instance()->setLogEntryNotyfyEnabled(params.toBool());
				return true;
			}
			if(method == M_GET_VERBOSITY) {
				return NecroLog::topicsLogTresholds();
			}
			if(method == M_SET_VERBOSITY) {
				const std::string &s = params.toString();
				NecroLog::setTopicsLogTresholds(s);
				return true;
			}
		}
		return Super::callMethod(shv_path, method, params);
	}
private:
	std::vector<cp::MetaMethod> m_metaMethods;
};
}

static const char M_RELOAD_CONFIG[] = "reloadConfig";
static const char M_RESTART[] = "restart";
//static const char M_MOUNT_POINT_FOR_CLIENT_ID[] = "mountPointForClientId";
static const char M_APP_VERSION[] = "appVersion";
static const char M_GIT_COMMIT[] = "gitCommit";
static const char M_BROKER_ID[] = "brokerId";
static const char M_MASTER_BROKER_ID[] = "masterBrokerId";

BrokerAppNode::BrokerAppNode(shv::iotqt::node::ShvNode *parent)
	: Super("", &m_metaMethods, parent)
	, m_metaMethods {
		{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
		{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
		{cp::Rpc::METH_PING, cp::MetaMethod::Signature::VoidVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
		{cp::Rpc::METH_ECHO, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
		{M_APP_VERSION, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_BROWSE},
		{M_GIT_COMMIT, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
		{M_BROKER_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
		{M_MASTER_BROKER_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
		{cp::Rpc::METH_SUBSCRIBE, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
		{cp::Rpc::METH_UNSUBSCRIBE, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
		{cp::Rpc::METH_REJECT_NOT_SUBSCRIBED, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_READ},
		{M_RELOAD_CONFIG, cp::MetaMethod::Signature::VoidVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE},
		{M_RESTART, cp::MetaMethod::Signature::VoidVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_SERVICE},
	}
{
	new BrokerLogNode(this);
}

chainpack::RpcValue BrokerAppNode::callMethodRq(const chainpack::RpcRequest &rq)
{
	const cp::RpcValue::String &shv_path = rq.shvPath().toString();
	if(shv_path.empty()) {
		const cp::RpcValue::String method = rq.method().toString();
		if(method == cp::Rpc::METH_SUBSCRIBE) {
			const shv::chainpack::RpcValue parms = rq.params();
			const shv::chainpack::RpcValue::Map &pm = parms.toMap();
			std::string path = pm.value(cp::Rpc::PAR_PATH).toString();
			std::string method = pm.value(cp::Rpc::PAR_METHOD).toString();
			int client_id = rq.peekCallerId();
			BrokerApp::instance()->addSubscription(client_id, path, method);
			return true;
		}
		if(method == cp::Rpc::METH_UNSUBSCRIBE) {
			const shv::chainpack::RpcValue parms = rq.params();
			const shv::chainpack::RpcValue::Map &pm = parms.toMap();
			std::string path = pm.value(cp::Rpc::PAR_PATH).toString();
			std::string method = pm.value(cp::Rpc::PAR_METHOD).toString();
			int client_id = rq.peekCallerId();
			return BrokerApp::instance()->removeSubscription(client_id, path, method);
		}
		if(method == cp::Rpc::METH_REJECT_NOT_SUBSCRIBED) {
			const shv::chainpack::RpcValue parms = rq.params();
			const shv::chainpack::RpcValue::Map &pm = parms.toMap();
			std::string path = pm.value(cp::Rpc::PAR_PATH).toString();
			std::string method = pm.value(cp::Rpc::PAR_METHOD).toString();
			int client_id = rq.peekCallerId();
			return BrokerApp::instance()->rejectNotSubscribedSignal(client_id, path, method);
		}
		if (method == M_MASTER_BROKER_ID) {
			auto *conn = BrokerApp::instance()->mainMasterBrokerConnection();
			if (!conn) {
				return std::string();
			}
			int rqid = conn->nextRequestId();
			shv::iotqt::rpc::RpcResponseCallBack *cb = new shv::iotqt::rpc::RpcResponseCallBack(conn, rqid, this);
			cb->start(this, [this, rq](const cp::RpcResponse &master_resp) {				cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
				if (master_resp.isSuccess()) {
					shvInfo() <<master_resp.result().toCpon();
					resp.setResult(master_resp.result());
				}
				else {
					shvError() << master_resp.errorString();
					resp.setError(master_resp.error());
				}
				emitSendRpcMessage(resp);
			});

			conn->callShvMethod(rqid, ".broker/app", "brokerId", cp::RpcValue());
			return cp::RpcValue();
		}
	}
	return Super::callMethodRq(rq);
}

shv::chainpack::RpcValue BrokerAppNode::callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == cp::Rpc::METH_PING) {
			return true;
		}
		if(method == cp::Rpc::METH_ECHO) {
			return params.isValid()? params: nullptr;
		}
		if(method == M_APP_VERSION) {
			return BrokerApp::applicationVersion().toStdString();
		}
		if(method == M_GIT_COMMIT) {
#ifdef GIT_COMMIT
			return SHV_EXPAND_AND_QUOTE(GIT_COMMIT);
#else
			return "N/A";
#endif
		}
		if(method == M_BROKER_ID) {
			return BrokerApp::instance()->brokerId();
		}
		if(method == M_RELOAD_CONFIG) {
			QTimer::singleShot(500, BrokerApp::instance(), &BrokerApp::reloadConfigRemountDevices);
			return true;
		}
		if(method == M_RESTART) {
			shvInfo() << "Server restart requested via RPC.";
			QTimer::singleShot(500, BrokerApp::instance(), &BrokerApp::quit);
			return true;
		}
	}
	return Super::callMethod(shv_path, method, params);
}

}}
