#include "brokeraclnode.h"
#include "brokerapp.h"

#include <shv/core/utils/shvpath.h>
#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpc.h>
#include <shv/core/string.h>
#include <shv/core/log.h>
#include <shv/core/exception.h>

namespace cp = shv::chainpack;

namespace shv {
namespace broker {

static const std::string GRANTS = "grants";
static const std::string WEIGHT = "weight";
static const std::string GRANT_NAME = "grantName";

//========================================================
// EtcAclNode
//========================================================
static std::vector<cp::MetaMethod> meta_methods_dir_ls {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};
static std::vector<cp::MetaMethod> meta_methods_property {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_SET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSetter, cp::Rpc::ROLE_CONFIG},
};

static const std::string M_VALUE = "value";
static const std::string M_SET_VALUE = "setValue";

static std::vector<cp::MetaMethod> meta_methods_acl_node {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{M_SET_VALUE, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_CONFIG},
};

static std::vector<cp::MetaMethod> meta_methods_acl_subnode {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{M_VALUE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
};

EtcAclNode::EtcAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("acl", &meta_methods_dir_ls, parent)
{
	{
		new FstabAclNode(this);
		//connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &BrokerAclNode::reload);
	}
	{
		new UsersAclNode(this);
		//connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &BrokerAclNode::reload);
	}
	{
		new RolesAclNode(this);
		//connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &BrokerAclNode::reload);
	}
	{
		new PathsAclNode(this);
		//connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &BrokerAclNode::reload);
	}
}

//enum AclAccessLevel {AclUserView = cp::MetaMethod::AccessLevel::Service, AclUserAdmin = cp::MetaMethod::AccessLevel::Admin};

BrokerAclNode::BrokerAclNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent)
	: Super(config_name, &meta_methods_dir_ls, parent)
{
}

std::vector<chainpack::MetaMethod> *BrokerAclNode::metaMethodsForPath(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 0)
		return &meta_methods_acl_node;
	if(shv_path.size() == 1)
		return &meta_methods_acl_subnode;
	if(shv_path.size() == 2)
		return &meta_methods_property;
	return &meta_methods_dir_ls;
}

size_t BrokerAclNode::methodCount(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	return metaMethodsForPath(shv_path)->size();
}

const chainpack::MetaMethod *BrokerAclNode::metaMethod(const iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	std::vector<cp::MetaMethod> *mm = metaMethodsForPath(shv_path);
	if(ix < mm->size())
		return &mm->at(ix);
	SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " on shv path: " + shv_path.join('/'));
}

//========================================================
// FstabAclNode
//========================================================
static const std::string ACL_FSTAB_DESCR = "description";
static const std::string ACL_FSTAB_MPOINT = "mountPoint";

FstabAclNode::FstabAclNode(iotqt::node::ShvNode *parent)
	: Super("fstab", parent)
{

}

iotqt::node::ShvNode::StringList FstabAclNode::childNames(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		BrokerApp *app = BrokerApp::instance();
		AclManager *mng = app->aclManager();
		return mng->mountDeviceIds();
	}
	else if(shv_path.size() == 1) {
		return iotqt::node::ShvNode::StringList{ACL_FSTAB_DESCR, ACL_FSTAB_MPOINT};
	}
	return Super::childNames(shv_path);
}

chainpack::RpcValue FstabAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(shv_path.size() == 2) {
		if(method == cp::Rpc::METH_GET) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			AclMountDef u = mng->mountDef(shv_path.value(0).toString());
			auto pn = shv_path.value(1);
			if(pn == ACL_FSTAB_DESCR)
				return u.description;
			if(pn == ACL_FSTAB_MPOINT)
				return u.mountPoint;
		}
	}
	return Super::callMethod(shv_path, method, params);
}

//========================================================
// RolesAclNode
//========================================================
static const std::string ACL_ROLE_NAME = "name";
static const std::string ACL_ROLE_WEIGHT = "weight";
static const std::string ACL_ROLE_ROLES = "roles";

RolesAclNode::RolesAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("roles", parent)
{
}

iotqt::node::ShvNode::StringList RolesAclNode::childNames(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		BrokerApp *app = BrokerApp::instance();
		AclManager *mng = app->aclManager();
		return mng->roles();
	}
	else if(shv_path.size() == 1) {
		return iotqt::node::ShvNode::StringList{ACL_ROLE_NAME, ACL_ROLE_WEIGHT, ACL_ROLE_ROLES};
	}
	return Super::childNames(shv_path);
}

chainpack::RpcValue RolesAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(shv_path.size() == 2) {
		if(method == cp::Rpc::METH_GET) {
			std::string role = shv_path.value(0).toString();
			auto pn = shv_path.value(1);
			if(pn == ACL_ROLE_NAME)
				return std::move(role);
			AclManager *mng = BrokerApp::instance()->aclManager();
			AclRole u = mng->role(role);
			if(pn == ACL_ROLE_WEIGHT)
				return u.weight;
			if(pn == ACL_ROLE_ROLES)
				return shv::chainpack::RpcValue::List::fromStringList(u.roles);
		}
	}
	return Super::callMethod(shv_path, method, params);
}
//========================================================
// UsersAclNode
//========================================================
static const std::string ACL_USER_NAME = "name";
static const std::string ACL_USER_PASSWORD = "password";
static const std::string ACL_USER_ROLES = "roles";
static const std::string ACL_USER_PASSWORD_FORMAT = "passwordFormat";

UsersAclNode::UsersAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("users", parent)
{
}

iotqt::node::ShvNode::StringList UsersAclNode::childNames(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		BrokerApp *app = BrokerApp::instance();
		AclManager *mng = app->aclManager();
		return mng->users();
	}
	else if(shv_path.size() == 1) {
		return iotqt::node::ShvNode::StringList{ACL_USER_NAME, ACL_USER_PASSWORD, ACL_USER_PASSWORD_FORMAT, ACL_USER_ROLES};
	}
	return Super::childNames(shv_path);
}

chainpack::RpcValue UsersAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(shv_path.size() == 0) {
		if(method == M_SET_VALUE) {
			if(params.isList()) {
				const auto &lst = params.toList();
				const std::string &name = lst.value(0).toString();
				auto user = AclUser::fromRpcValue(lst.value(1));
				AclManager *mng = BrokerApp::instance()->aclManager();
				mng->setUser(name, user);
			}
		}
	}
	if(shv_path.size() == 1) {
		if(method == M_VALUE) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			AclUser u = mng->user(shv_path.value(0).toString());
			return u.toRpcValueMap();
		}
	}
	if(shv_path.size() == 2) {
		if(method == cp::Rpc::METH_GET) {
			std::string user = shv_path.value(0).toString();
			auto pn = shv_path.value(1);
			if(pn == ACL_USER_NAME)
				return std::move(user);

			AclManager *mng = BrokerApp::instance()->aclManager();
			AclUser u = mng->user(user);
			if(pn == ACL_USER_PASSWORD)
				return u.password.password;
			if(pn == ACL_USER_PASSWORD_FORMAT)
				return AclPassword::formatToString(u.password.format);
			if(pn == ACL_USER_ROLES)
				return shv::chainpack::RpcValue::List::fromStringList(u.roles);
		}
	}
	return Super::callMethod(shv_path, method, params);
}

//========================================================
// PathsAclNode
//========================================================

static const std::string ACL_PATHS_GRANT = "grant";
static const std::string ACL_PATHS_GRANT_TYPE = "type";
//static const std::string ACL_PATHS_GRANT_FWD = cp::PathAccessGrant::FORWARD_USER_LOGIN;

PathsAclNode::PathsAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("paths", parent)
{

}

std::vector<chainpack::MetaMethod> *PathsAclNode::metaMethodsForPath(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 0)
		return &meta_methods_acl_node;
	if(shv_path.size() == 1)
		return &meta_methods_acl_subnode;
	if(shv_path.size() == 2)
		return &meta_methods_dir_ls;
	if(shv_path.size() == 3)
		return &meta_methods_property;
	return &meta_methods_dir_ls;
}

iotqt::node::ShvNode::StringList PathsAclNode::childNames(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		AclManager *mng = BrokerApp::instance()->aclManager();
		return mng->pathsRoles();
	}
	else if(shv_path.size() == 1) {
		AclManager *mng = BrokerApp::instance()->aclManager();
		AclRolePaths role_paths = mng->pathsRolePaths(shv_path.value(0).toString());
		iotqt::node::ShvNode::StringList ret;
		for(auto s : shv::chainpack::Utils::mapKeys(role_paths))
			ret.push_back('\'' + s + '\'');
		return ret;
	}
	else if(shv_path.size() == 2) {
		return iotqt::node::ShvNode::StringList{ACL_PATHS_GRANT, ACL_PATHS_GRANT_TYPE,/* ACL_PATHS_GRANT_FWD*/};
	}
	return Super::childNames(shv_path);
}

chainpack::RpcValue PathsAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(shv_path.size() == 3) {
		if(method == cp::Rpc::METH_GET) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			AclRolePaths role_paths = mng->pathsRolePaths(shv_path.value(0).toString());
			std::string path = shv_path.value(1).toString();
			if(path.size() > 1 && path[0] == '\'' && path[path.size() - 1] == '\'')
				path = path.substr(1, path.size() - 2);
			auto it = role_paths.find(path);
			if(it == role_paths.end())
				SHV_EXCEPTION("Invalid path: " + path);
			auto g = it->second;
			std::string pn = shv_path.value(2).toString();
			if(pn == ACL_PATHS_GRANT)
				return g.toRpcValue();
			if(pn == ACL_PATHS_GRANT_TYPE)
				return cp::PathAccessGrant::typeToString(g.type);
			//if(pn == ACL_PATHS_GRANT_FWD)
			//	return g.forwardUserLoginFromRequest;
		}
	}
	return Super::callMethod(shv_path, method, params);
}

}}
