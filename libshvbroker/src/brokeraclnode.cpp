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

//static const char M_ADD_USER[] = "addUser";
//static const char M_DEL_USER[] = "delUser";

static const char M_ADD_GRANT[] = "addGrant";
static const char M_EDIT_GRANT[] = "editGrant";
static const char M_DEL_GRANT[] = "delGrant";

static const char M_SET_GRANT_PATHS[] = "setGrantPaths";
static const char M_DEL_GRANT_PATHS[] = "delGrantPaths";
static const char M_GET_GRANT_PATHS[] = "getGrantPaths";

static std::vector<cp::MetaMethod> meta_methods_acl_grants {
	{M_ADD_GRANT, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_CONFIG},
	{M_EDIT_GRANT, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_CONFIG},
	{M_DEL_GRANT, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_CONFIG}
};

static std::vector<cp::MetaMethod> meta_methods_acl_paths {
	{M_SET_GRANT_PATHS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_CONFIG},
	{M_DEL_GRANT_PATHS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_CONFIG},
	{M_GET_GRANT_PATHS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_CONFIG}
};

EtcAclNode::EtcAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("acl", &meta_methods_dir_ls, parent)
{
	{
		auto *nd = new BrokerAclNode("fstab", this);
		connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &BrokerAclNode::reload);
	}
	{
		auto *nd = new UsersAclNode(this);
		connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &BrokerAclNode::reload);
	}
	{
		auto *nd = new RolesAclNode(this);
		connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &BrokerAclNode::reload);
	}
	{
		auto *nd = new PathsAclNode(this);
		connect(BrokerApp::instance(), &BrokerApp::configReloaded, nd, &BrokerAclNode::reload);
	}
}

//enum AclAccessLevel {AclUserView = cp::MetaMethod::AccessLevel::Service, AclUserAdmin = cp::MetaMethod::AccessLevel::Admin};

BrokerAclNode::BrokerAclNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent)
	: Super(config_name, &meta_methods_dir_ls, parent)
{
}
/*
void BrokerConfigFileNode::loadValues()
{
	m_values = BrokerApp::instance()->aclConfig(nodeId(), !shv::core::Exception::Throw);
	Super::loadValues();
}

void BrokerConfigFileNode::saveValues()
{
	BrokerApp::instance()->setAclConfig(nodeId(), m_values, shv::core::Exception::Throw);
}
*/
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

size_t RolesAclNode::methodCount(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 2)
		return meta_methods_property.size();
	//return Super::methodCount(shv_path);
	return meta_methods_dir_ls.size();
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

const chainpack::MetaMethod *RolesAclNode::metaMethod(const iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	auto &mm = (shv_path.size() == 2)? meta_methods_property: meta_methods_dir_ls;
	if(ix < mm.size())
		return &mm.at(ix);
	SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " on shv path: " + shv_path.join('/'))
}

chainpack::RpcValue RolesAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(shv_path.size() == 2) {
		if(method == cp::Rpc::METH_GET) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			chainpack::AclRole u = mng->role(shv_path.value(0).toString());
			auto pn = shv_path.value(1);
			if(pn == ACL_ROLE_NAME)
				return u.name;
			if(pn == ACL_ROLE_WEIGHT)
				return u.weight;
			if(pn == ACL_ROLE_ROLES)
				return shv::chainpack::RpcValue::List::fromStringList(u.roles);
		}
	}
	return Super::callMethod(shv_path, method, params);
}
#if 0
size_t GrantsAclNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods_acl_grants.size() + Super::methodCount(shv_path) : Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *GrantsAclNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		size_t all_method_count = meta_methods_acl_grants.size() + Super::methodCount(shv_path);

		if (ix < meta_methods_acl_grants.size())
			return &(meta_methods_acl_grants[ix]);
		else if (ix < all_method_count)
			return Super::metaMethod(shv_path, ix - meta_methods_acl_grants.size());
		else
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(all_method_count))
	}

	return Super::metaMethod(shv_path, ix);
}
shv::chainpack::RpcValue GrantsAclNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	if(shv_path.empty()) {
		if(method == M_ADD_GRANT) {
			return addGrant(params);
		}
		else if(method == M_EDIT_GRANT) {
			return editGrant(params);
		}
		else if(method == M_DEL_GRANT) {
			return delGrant(params);
		}
	}

	return Super::callMethod(shv_path, method, params);
}

bool GrantsAclNode::addGrant(const shv::chainpack::RpcValue &params)
{
	if (!params.isMap() || !params.toMap().hasKey(GRANT_NAME)){
		SHV_EXCEPTION("Invalid parameters format. Params must be a RpcValue::Map and it must contains key: grantName.");
	}

	cp::RpcValue::Map map = params.toMap();
	std::string grant_name = map.value(GRANT_NAME).toStdString();

	cp::RpcValue::Map grants_config = values().toMap();

	if (grants_config.hasKey(grant_name)){
		SHV_EXCEPTION("Grant " + grant_name + " already exists");
	}

	cp::RpcValue::Map new_grant;

	if (map.hasKey(GRANTS)){
		const cp::RpcValue grants = map.value(GRANTS);
		if (grants.isList()){
			new_grant[GRANTS] = grants;
		}
		else{
			SHV_EXCEPTION("Invalid parameters format. Param grants must be a RpcValue::List.");
		}
	}
	if (map.hasKey(WEIGHT)){
		const cp::RpcValue weight = map.value(WEIGHT);
		if (weight.isInt()){
			new_grant[WEIGHT] = weight;
		}
		else{
			SHV_EXCEPTION("Invalid parameters format. Param weight must be a RpcValue::Int.");
		}
	}

	m_values.set(grant_name, new_grant);
	commitChanges();

	return true;
}

bool GrantsAclNode::editGrant(const shv::chainpack::RpcValue &params)
{
	if (!params.isMap() || !params.toMap().hasKey(GRANT_NAME)){
		SHV_EXCEPTION("Invalid parameters format. Params must be a RpcValue::Map and it must contains key: grantName.");
	}

	cp::RpcValue::Map params_map = params.toMap();
	std::string grant_name = params_map.value(GRANT_NAME).toStdString();

	cp::RpcValue::Map grants_config = values().toMap();

	if (!grants_config.hasKey(grant_name)){
		SHV_EXCEPTION("Grant " + grant_name + " does not exist.");
	}

	cp::RpcValue::Map new_grant;

	if (params_map.hasKey(GRANTS)){
		const cp::RpcValue grants = params_map.value(GRANTS);
		if (grants.isList()){
			new_grant[GRANTS] = grants;
		}
		else{
			SHV_EXCEPTION("Invalid parameters format. Param grants must be a RpcValue::List.");
		}
	}

	if (params_map.hasKey(WEIGHT)){
		const cp::RpcValue weight = params_map.value(WEIGHT);
		if (weight.isInt()){
			new_grant[WEIGHT] = weight;
		}
		else{
			SHV_EXCEPTION("Invalid parameters format. Param weight must be a RpcValue::Int.");
		}
	}

	m_values.set(grant_name, new_grant);
	commitChanges();

	return true;
}

bool GrantsAclNode::delGrant(const shv::chainpack::RpcValue &params)
{
	if (!params.isString() || params.toString().empty()){
		SHV_EXCEPTION("Invalid parameters format. Param must be non empty RpcValue::String.");
	}

	std::string grant_name = params.toString();
	const cp::RpcValue::Map &grants_config = values().toMap();

	if (!grants_config.hasKey(grant_name)){
		SHV_EXCEPTION("Grant " + grant_name + " does not exist.");
	}

	m_values.set(grant_name, cp::RpcValue());
	commitChanges();

	return true;
}
#endif
//========================================================
// UsersAclNode
//========================================================
/*
static std::vector<cp::MetaMethod> meta_methods_acl_user {
	{M_ADD_USER, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_CONFIG},
	{M_DEL_USER, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::ROLE_CONFIG}
};
*/
static const std::string ACL_USER_NAME = "name";
static const std::string ACL_USER_PASSWORD = "password";
static const std::string ACL_USER_ROLES = "roles";
static const std::string ACL_USER_PASSWORD_FORMAT = "passwordFormat";

UsersAclNode::UsersAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("users", parent)
{
}

size_t UsersAclNode::methodCount(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 2)
		return meta_methods_property.size();
	//return Super::methodCount(shv_path);
	return meta_methods_dir_ls.size();
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

const chainpack::MetaMethod *UsersAclNode::metaMethod(const iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	auto &mm = (shv_path.size() == 2)? meta_methods_property: meta_methods_dir_ls;
	if(ix < mm.size())
		return &mm.at(ix);
	SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " on shv path: " + shv_path.join('/'))
}

chainpack::RpcValue UsersAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(shv_path.size() == 2) {
		if(method == cp::Rpc::METH_GET) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			chainpack::AclUser u = mng->user(shv_path.value(0).toString());
			auto pn = shv_path.value(1);
			if(pn == ACL_USER_NAME)
				return u.name;
			if(pn == ACL_USER_PASSWORD)
				return u.password.password;
			if(pn == ACL_USER_PASSWORD_FORMAT)
				return chainpack::AclPassword::formatToString(u.password.format);
			if(pn == ACL_USER_ROLES)
				return shv::chainpack::RpcValue::List::fromStringList(u.roles);
		}
	}
	return Super::callMethod(shv_path, method, params);
}

#if 0
size_t UsersAclNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods_acl_users.size() + Super::methodCount(shv_path) : Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *UsersAclNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		size_t all_method_count = meta_methods_acl_users.size() + Super::methodCount(shv_path);

		if (ix < meta_methods_acl_users.size())
			return &(meta_methods_acl_users[ix]);
		else if (ix < all_method_count)
			return Super::metaMethod(shv_path, ix - meta_methods_acl_users.size());
		else
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(all_method_count))
	}

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue UsersAclNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	/*
	if(shv_path.empty()) {
		if(method == M_ADD_USER) {
			return addUser(params);
		}
		else if(method == M_DEL_USER) {
			return delUser(params);
		}
	}
	*/
	return Super::callMethod(shv_path, method, params);
}

bool UsersAclNode::addUser(const shv::chainpack::RpcValue &params)
{
	if (!params.isMap() || !params.toMap().hasKey("user") || !params.toMap().hasKey("password")){
		SHV_EXCEPTION("Invalid parameters format. Params must be a RpcValue::Map and it must contains keys: user, password.");
	}

	cp::RpcValue::Map map = params.toMap();
	std::string user_name = map.value("user").toStdString();

	cp::RpcValue::Map users_config = values().toMap();

	if (users_config.hasKey(user_name)){
		SHV_EXCEPTION("User " + user_name + " already exists");
	}

	cp::RpcValue::Map user;
	const cp::RpcValue grants = map.value(GRANTS);
	user[GRANTS] = (grants.isList()) ? grants.toList() : cp::RpcValue::List();
	user["password"] = map.value("password").toStdString();
	user["passwordFormat"] = "sha1";

	m_values.set(user_name, user);
	commitChanges();

	return true;
}

bool UsersAclNode::delUser(const shv::chainpack::RpcValue &params)
{
	if (!params.isString() || params.toString().empty()){
		SHV_EXCEPTION("Invalid parameters format. Param must be non empty string.");
	}

	std::string user_name = params.toString();
	const cp::RpcValue::Map &users_config = values().toMap();

	if (!users_config.hasKey(user_name)){
		SHV_EXCEPTION("User " + user_name + " does not exist.");
	}

	m_values.set(user_name, cp::RpcValue());
	commitChanges();

	return true;
}
#endif
//========================================================
// PathsAclNode
//========================================================

static const std::string ACL_PATHS_GRANT = "grant";
static const std::string ACL_PATHS_GRANT_TYPE = "type";
static const std::string ACL_PATHS_GRANT_FWD = "forward";

PathsAclNode::PathsAclNode(shv::iotqt::node::ShvNode *parent)
	: Super("paths", parent)
{

}

size_t PathsAclNode::methodCount(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 3)
		return meta_methods_property.size();
	return meta_methods_dir_ls.size();
}

iotqt::node::ShvNode::StringList PathsAclNode::childNames(const iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.empty()) {
		AclManager *mng = BrokerApp::instance()->aclManager();
		return mng->pathsRoles();
	}
	else if(shv_path.size() == 1) {
		AclManager *mng = BrokerApp::instance()->aclManager();
		chainpack::AclRolePaths role_paths = mng->pathsRolePaths(shv_path.value(0).toString());
		iotqt::node::ShvNode::StringList ret;
		for(auto s : shv::chainpack::Utils::mapKeys(role_paths))
			ret.push_back('\'' + s + '\'');
		return ret;
	}
	else if(shv_path.size() == 2) {
		return iotqt::node::ShvNode::StringList{ACL_PATHS_GRANT, ACL_PATHS_GRANT_TYPE, ACL_PATHS_GRANT_FWD};
	}
	return Super::childNames(shv_path);
}

const chainpack::MetaMethod *PathsAclNode::metaMethod(const iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	auto &mm = (shv_path.size() == 3)? meta_methods_property: meta_methods_dir_ls;
	if(ix < mm.size())
		return &mm.at(ix);
	SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " on shv path: " + shv_path.join('/'))
}

chainpack::RpcValue PathsAclNode::callMethod(const iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(shv_path.size() == 3) {
		if(method == cp::Rpc::METH_GET) {
			AclManager *mng = BrokerApp::instance()->aclManager();
			chainpack::AclRolePaths role_paths = mng->pathsRolePaths(shv_path.value(0).toString());
			std::string path = shv_path.value(1).toString();
			if(path.size() > 1 && path[0] == '\'' && path[path.size() - 1] == '\'')
				path = path.substr(1, path.size() - 2);
			auto it = role_paths.find(path);
			if(it == role_paths.end())
				SHV_EXCEPTION("Invalid path: " + path)
			auto g = it->second;
			std::string pn = shv_path.value(2).toString();
			if(pn == ACL_PATHS_GRANT)
				return g.toRpcValue();
			if(pn == ACL_PATHS_GRANT_TYPE)
				return cp::PathAccessGrant::typeToString(g.type);
			if(pn == ACL_PATHS_GRANT_FWD)
				return g.forwardGrantFromRequest;
		}
	}
	return Super::callMethod(shv_path, method, params);
}
#if 0
size_t PathsAclNode::methodCount(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	return (shv_path.empty()) ? meta_methods_acl_paths.size() + Super::methodCount(shv_path) : Super::methodCount(shv_path);
}

const shv::chainpack::MetaMethod *PathsAclNode::metaMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty()) {
		size_t all_method_count = meta_methods_acl_paths.size() + Super::methodCount(shv_path);

		if (ix < meta_methods_acl_paths.size())
			return &(meta_methods_acl_paths[ix]);
		else if (ix < all_method_count)
			return Super::metaMethod(shv_path, ix - meta_methods_acl_paths.size());
		else
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(all_method_count))
	}

	return Super::metaMethod(shv_path, ix);
}

shv::chainpack::RpcValue PathsAclNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params)
{
	/*
	if(shv_path.empty()) {
		if(method == M_SET_GRANT_PATHS) {
			return setGrantPaths(params);
		}
		else if(method == M_DEL_GRANT_PATHS) {
			return delGrantPaths(params);
		}
		else if(method == M_GET_GRANT_PATHS) {
			return getGrantPaths(params);
		}
	}
	*/
	return Super::callMethod(shv_path, method, params);
}

bool PathsAclNode::setGrantPaths(const shv::chainpack::RpcValue &params)
{
	if (!params.isList() || (params.toList().size() != 2)){
		SHV_EXCEPTION("Invalid parameters format. Params must be RpcValue::List with 2 items [grant_name, paths]");
	}

	const cp::RpcValue::List &params_lst = params.toList();

	if (!params_lst.value(0).isString() || (!params_lst.value(1).isMap())){
		SHV_EXCEPTION("Invalid parameters format. Types for items must be [RpcValue::String, RpcValue::Map]");
	}

	std::string grant_name = params_lst.value(0).toStdString();
	const cp::RpcValue::Map &paths = params_lst.value(1).toMap();

	m_values.set(grant_name, !paths.empty() ? paths : cp::RpcValue());
	commitChanges();

	return true;
}

bool PathsAclNode::delGrantPaths(const shv::chainpack::RpcValue &params)
{
	if (!params.isString() || params.toString().empty()){
		SHV_EXCEPTION("Invalid parameters format. Param must be non empty RpcValue::String.");
	}

	std::string grant_name = params.toString();
	const cp::RpcValue::Map &paths_config = values().toMap();

	if (paths_config.hasKey(grant_name)){
		m_values.set(grant_name, cp::RpcValue());
		commitChanges();
	}

	return true;
}

shv::chainpack::RpcValue PathsAclNode::getGrantPaths(const shv::chainpack::RpcValue &params)
{
	if (!params.isString() || params.toString().empty()){
		SHV_EXCEPTION("Invalid parameters format. Param must be non empty RpcValue::String.");
	}

	std::string grant_name = params.toString();
	const cp::RpcValue::Map &paths_config = values().toMap();

	return (paths_config.hasKey(grant_name)) ? paths_config.value(grant_name) : cp::RpcValue::Map();
}

shv::chainpack::RpcValue PathsAclNode::valueOnPath(const shv::iotqt::node::ShvNode::StringViewList &shv_path, bool throw_exc)
{
	//shvInfo() << "valueOnPath:" << shv_path.join('/');
	shv::chainpack::RpcValue v = values();
	for (size_t i = 0; i < shv_path.size(); ++i) {
		shv::iotqt::node::ShvNode::StringView dir = shv_path[i];
		if(i == 1)
			dir = dir.mid(1, dir.length() - 2);
		const shv::chainpack::RpcValue::Map &m = v.toMap();
		std::string key = dir.toString();
		v = m.value(key);
		//shvInfo() << "\t i:" << i << "key:" << key << "val:" << v.toCpon();
		if(!v.isValid()) {
			if(throw_exc)
				SHV_EXCEPTION("Invalid path: " + shv_path.join('/'));
			return v;
		}
	}
	return v;
}
shv::iotqt::node::ShvNode::StringList PathsAclNode::childNames(const shv::iotqt::node::ShvNode::StringViewList &shv_path)
{
	if(shv_path.size() == 1) {
		std::vector<std::string> keys = values().at(shv_path[0].toString()).toMap().keys();
		for (size_t i = 0; i < keys.size(); ++i)
			keys[i] = shv::core::utils::ShvPath::SHV_PATH_QUOTE + keys[i] + shv::core::utils::ShvPath::SHV_PATH_QUOTE;

		return keys;
	}
	else {
		return Super::childNames(shv_path);
	}
}
#endif

}}
