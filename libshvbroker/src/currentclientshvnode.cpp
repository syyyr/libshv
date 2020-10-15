#include "currentclientshvnode.h"
#include "brokerapp.h"
#include "rpc/clientconnection.h"

namespace cp = shv::chainpack;

using namespace std;

static string M_CLIENT_ID = "clientId";
static string M_MOUNT_POINT = "mountPoint";
static string M_USER_ROLES = "userRoles";
static string M_USER_PROFILE = "userProfile";

const char *CurrentClientShvNode::NodeId = "currentClient";

CurrentClientShvNode::CurrentClientShvNode(shv::iotqt::node::ShvNode *parent)
	: Super(NodeId, &m_metaMethods, parent)
	, m_metaMethods {
		{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
		{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
		{M_CLIENT_ID, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_MOUNT_POINT, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_USER_ROLES, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
		{M_USER_PROFILE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
	}
{
}

shv::chainpack::RpcValue CurrentClientShvNode::callMethodRq(const shv::chainpack::RpcRequest &rq)
{
	shv::chainpack::RpcValue shv_path = rq.shvPath();
	if(shv_path.toString().empty()) {
		const shv::chainpack::RpcValue::String &method = rq.method().toString();
		if(method == M_CLIENT_ID) {
			int client_id = rq.peekCallerId();
			return client_id;
		}
		if(method == M_MOUNT_POINT) {
			int client_id = rq.peekCallerId();
			auto *cli = shv::broker::BrokerApp::instance()->clientById(client_id);
			if(cli) {
				return cli->mountPoint();
			}
			return nullptr;
		}
		if(method == M_USER_ROLES) {
			int client_id = rq.peekCallerId();
			auto *app = shv::broker::BrokerApp::instance();
			auto *cli = app->clientById(client_id);
			if(cli) {
				const string user_name = cli->userName();
				std::vector<shv::broker::AclManager::FlattenRole> roles = app->aclManager()->userFlattenRoles(user_name);
				cp::RpcValue::List ret;
				std::transform(roles.begin(), roles.end(), std::back_inserter(ret), [](const shv::broker::AclManager::FlattenRole &r) -> cp::RpcValue {
					return r.name;
				});
				return ret;
			}
			return nullptr;
		}
		if(method == M_USER_PROFILE) {
			int client_id = rq.peekCallerId();
			auto *app = shv::broker::BrokerApp::instance();
			auto *cli = app->clientById(client_id);
			if(cli) {
				const string user_name = cli->userName();
				cp::RpcValue ret = app->aclManager()->userProfile(user_name);
				return ret.isValid()? ret: nullptr;
			}
			return nullptr;
		}
	}
	return Super::callMethodRq(rq);
}
