#include "aclmanager.h"
#include "brokerapp.h"

#include <shv/chainpack/cponreader.h>
#include <shv/coreqt/log.h>

#include <QCryptographicHash>

#include <fstream>

#define logAclManagerD() nCDebug("AclManager")
#define logAclManagerM() nCMessage("AclManager")
#define logAclManagerI() nCInfo("AclManager")

namespace cp = shv::chainpack;

namespace shv {
namespace broker {
namespace {
static std::string sha1_hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(s.data(), (int)s.length());
	return std::string(hash.result().toHex().constData());
}
}
//================================================================
// AclManagerBase
//================================================================
shv::broker::AclManager::AclManager(shv::broker::BrokerApp *broker_app)
	: QObject(broker_app)
	, m_brokerApp(broker_app)
{
}

std::vector<std::string> AclManager::mountDeviceIds()
{
	if(m_cache.aclMountDefs.empty()) {
		for(auto id : aclMountDeviceIds())
			m_cache.aclMountDefs[id];
	}
	return cp::Utils::mapKeys(m_cache.aclMountDefs);
}

AclMountDef AclManager::mountDef(const std::string &device_id)
{
	if(m_cache.aclMountDefs.empty())
		mountDeviceIds();
	auto it = m_cache.aclMountDefs.find(device_id);
	if(it == m_cache.aclMountDefs.end())
		return AclMountDef();
	if(!it->second.isValid()) {
		it->second = aclMountDef(device_id);
	}
	return it->second;
}

void AclManager::setMountDef(const std::string &device_id, const AclMountDef &v)
{
	aclSetMountDef(device_id, v);
	m_cache.aclMountDefs.clear();
}

std::vector<std::string> AclManager::users()
{
	if(m_cache.aclUsers.empty()) {
		for(auto id : aclUsers())
			m_cache.aclUsers[id];
	}
	return cp::Utils::mapKeys(m_cache.aclUsers);
}

AclUser AclManager::user(const std::string &user_name)
{
	if(m_cache.aclUsers.empty())
		users();
	auto it = m_cache.aclUsers.find(user_name);
	if(it == m_cache.aclUsers.end())
		return AclUser();
	if(!it->second.isValid()) {
		it->second = aclUser(user_name);
	}
	return it->second;
}

void AclManager::setUser(const std::string &user_name, const AclUser &u)
{
	aclSetUser(user_name, u);
	m_cache.aclUsers.clear();
	m_cache.userFlattenRoles.clear();
}

std::vector<std::string> AclManager::roles()
{
	if(m_cache.aclRoles.empty()) {
		for(auto id : aclRoles())
			m_cache.aclRoles[id];
	}
	return cp::Utils::mapKeys(m_cache.aclRoles);
}

AclRole AclManager::role(const std::string &role_name)
{
	if(m_cache.aclRoles.empty())
		roles();
	auto it = m_cache.aclRoles.find(role_name);
	if(it == m_cache.aclRoles.end())
		return AclRole();
	if(!it->second.isValid()) {
		it->second = aclRole(role_name);
	}
	return it->second;
}

void AclManager::setRole(const std::string &role_name, const AclRole &v)
{
	aclSetRole(role_name, v);
	m_cache.aclRoles.clear();
	m_cache.userFlattenRoles.clear();
}

std::vector<std::string> AclManager::accessRoles()
{
	if(m_cache.aclPathsRoles.empty()) {
		for(auto id : aclAccessRoles())
			m_cache.aclPathsRoles[id];
	}
	return cp::Utils::mapKeys(m_cache.aclPathsRoles);
}

AclRoleAccessRules AclManager::accessRoleRules(const std::string &role_name)
{
	if(m_cache.aclPathsRoles.empty())
		accessRoles();
	auto it = m_cache.aclPathsRoles.find(role_name);
	if(it == m_cache.aclPathsRoles.end())
		return AclRoleAccessRules();
	if(std::get<1>(it->second) == false) {
		it->second = std::pair<AclRoleAccessRules, bool>(aclAccessRoleRules(role_name), true);
	}
	return std::get<0>(it->second);
}

void AclManager::setAccessRoleRules(const std::string &role_name, const AclRoleAccessRules &v)
{
	aclSetAccessRoleRules(role_name, v);
	m_cache.aclPathsRoles.clear();
}

chainpack::UserLoginResult AclManager::checkPassword(const chainpack::UserLoginContext &login_context)
{
	using LoginType = cp::UserLogin::LoginType;
	chainpack::UserLogin login = login_context.userLogin();
	AclUser acl_user = user(login.user);
	if(!acl_user.isValid()) {
		shvError() << "Invalid user name:" << login.user;
		return cp::UserLoginResult(false, "Invalid user name.");
	}
	auto acl_pwd = acl_user.password;
	//std::string pwdsrv = std::get<0>(pwdt);
	//PasswordFormat pwdsrvfmt = std::get<1>(pwdt);
	if(acl_pwd.password.empty()) {
		shvError() << "Empty password not allowed:" << login.user;
		return cp::UserLoginResult(false, "Empty password not allowed.");
	}
	auto usr_login_type = login.loginType;
	if(usr_login_type == LoginType::Invalid)
		usr_login_type = LoginType::Sha1;

	std::string usr_pwd = login.password;
	if(usr_login_type == LoginType::Plain) {
		if(acl_pwd.format == AclPassword::Format::Plain) {
			if(acl_pwd.password == usr_pwd)
				return cp::UserLoginResult(true);
			return cp::UserLoginResult(false, "Invalid password.");
		}
		if(acl_pwd.format == AclPassword::Format::Sha1) {
			if(acl_pwd.password == sha1_hex(usr_pwd))
				return cp::UserLoginResult(true);
			return cp::UserLoginResult(false, "Invalid password.");
		}
	}
	if(usr_login_type == LoginType::Sha1) {
		/// login_type == "SHA1" is default
		if(acl_pwd.format == AclPassword::Format::Plain)
			acl_pwd.password = sha1_hex(acl_pwd.password);

		std::string nonce = login_context.serverNounce + acl_pwd.password;
		//shvWarning() << m_pendingAuthNonce << "prd" << nonce;
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
		hash.addData(nonce.data(), nonce.length());
		std::string correct_sha1 = std::string(hash.result().toHex().constData());
		//shvInfo() << nonce_sha1 << "vs" << sha1;
		if(usr_pwd == correct_sha1)
			return cp::UserLoginResult(true);
		return cp::UserLoginResult(false, "Invalid password.");
	}
	shvError() << "Unsupported login type" << cp::UserLogin::loginTypeToString(login.loginType) << "for user:" << login.user;
	return cp::UserLoginResult(false, "Invalid login type.");
}

std::string AclManager::mountPointForDevice(const chainpack::RpcValue &device_id)
{
	return mountDef(device_id.toString()).mountPoint;
}

void AclManager::reload()
{
	clearCache();
}

void AclManager::aclSetMountDef(const std::string &device_id, const AclMountDef &md)
{
	Q_UNUSED(device_id)
	Q_UNUSED(md)
	SHV_EXCEPTION("Mount points definition is read only.");
}

void AclManager::aclSetUser(const std::string &user_name, const AclUser &u)
{
	Q_UNUSED(user_name)
	Q_UNUSED(u)
	SHV_EXCEPTION("Users definition is read only.");
}

void AclManager::aclSetRole(const std::string &role_name, const AclRole &r)
{
	Q_UNUSED(role_name)
	Q_UNUSED(r)
	SHV_EXCEPTION("Roles definition is read only.");
}

void AclManager::aclSetAccessRoleRules(const std::string &role_name, const AclRoleAccessRules &rp)
{
	Q_UNUSED(role_name)
	Q_UNUSED(rp)
	SHV_EXCEPTION("Role paths definition is read only.");
}

std::map<std::string, AclManager::FlattenRole> AclManager::flattenRole_helper(const std::string &role_name, int nest_level)
{
	//shvInfo() << __FUNCTION__ << "role:" << role_name << "nest level:" << nest_level;
	std::map<std::string, FlattenRole> ret;
	AclRole ar = aclRole(role_name);
	if(ar.isValid()) {
		FlattenRole fr{role_name, ar.weight, nest_level};
		ret[role_name] = std::move(fr);
		for(auto rl : ar.roles) {
			//shvInfo() << "\t child role:" << rl;
			auto it = ret.find(rl);
			if(it != ret.end()) {
				shvWarning() << "Cyclic reference in roles detected for name:" << rl;
				continue;
			}
			auto ret2 = flattenRole_helper(rl, ++nest_level);
			ret.insert(ret2.begin(), ret2.end());
		}
	}
	else {
		shvWarning() << "role:" << role_name << "is not defined";
	}
	//shvInfo() << "\t ret:";
	//for(auto kv : ret)
	//	shvInfo() << "\t\t" << kv.first << kv.second.name;
	return ret;
}

std::vector<AclManager::FlattenRole> AclManager::userFlattenRoles(const std::string &user_name)
{
	if(m_cache.userFlattenRoles.find(user_name) == m_cache.userFlattenRoles.end()) {
		AclUser user_def = aclUser(user_name);
		if(!user_def.isValid())
			return std::vector<FlattenRole>();

		std::map<std::string, AclManager::FlattenRole> unique_roles;
		for(auto role : user_def.roles) {
			auto gg = flattenRole_helper(role, 1);
			unique_roles.insert(gg.begin(), gg.end());
		}
		std::vector<AclManager::FlattenRole> lst;
		for(const auto &kv : unique_roles)
			lst.push_back(kv.second);
		std::sort(lst.begin(), lst.end(), [](const FlattenRole &r1, const FlattenRole &r2) {
			if(r1.weight == r2.weight)
				return r1.nestLevel < r2.nestLevel;
			return r1.weight > r2.weight;
		});
		m_cache.userFlattenRoles[user_name] = lst;
	}
	return m_cache.userFlattenRoles[user_name];
}

std::vector<AclManager::FlattenRole> AclManager::flattenRole(const std::string &role)
{
	std::string key = "_Role#Key:" + role;
	if(m_cache.userFlattenRoles.find(key) == m_cache.userFlattenRoles.end()) {
		std::map<std::string, AclManager::FlattenRole> unique_roles;
		auto gg = flattenRole_helper(role, 1);
		unique_roles.insert(gg.begin(), gg.end());
		std::vector<AclManager::FlattenRole> lst;
		for(const auto &kv : unique_roles)
			lst.push_back(kv.second);
		std::sort(lst.begin(), lst.end(), [](const FlattenRole &r1, const FlattenRole &r2) {
			if(r1.weight == r2.weight)
				return r1.nestLevel < r2.nestLevel;
			return r1.weight > r2.weight;
		});
		m_cache.userFlattenRoles[key] = lst;
	}
	return m_cache.userFlattenRoles[key];
}

//================================================================
// AclManagerConfigFiles
//================================================================
const auto FILE_ACL_MOUNTS = std::string("mounts.cpon");
const auto FILE_ACL_MOUNTS_DEPR = std::string("fstab.cpon");

const auto FILE_ACL_USERS = std::string("users.cpon");

const auto FILE_ACL_ROLES = std::string("roles.cpon");
const auto FILE_ACL_ROLES_DEPR = std::string("grants.cpon");

const auto FILE_ACL_ACCESS = std::string("access.cpon");
const auto FILE_ACL_ACCESS_DEPR = std::string("paths.cpon");

AclManagerConfigFiles::AclManagerConfigFiles(BrokerApp *broker_app)
	: Super(broker_app)
{
	shvInfo() << "Creating ConfigFiles ACL manager";
}

std::string AclManagerConfigFiles::configDir() const
{
	return m_brokerApp->cliOptions()->configDir();
}

void AclManagerConfigFiles::clearCache()
{
	Super::clearCache();
	m_configFiles.clear();
}

chainpack::RpcValue AclManagerConfigFiles::aclConfig(const std::string &config_name, bool throw_exc)
{
	shv::chainpack::RpcValue config = m_configFiles.value(config_name);
	if(!config.isValid()) {
		config = loadAclConfig(config_name, throw_exc);
		m_configFiles[config_name] = config;
		if(!config.isValid()) {
			if(throw_exc)
				throw std::runtime_error("Config " + config_name + " does not exist.");
			else
				return chainpack::RpcValue();
		}
	}
	return config;
}

shv::chainpack::RpcValue AclManagerConfigFiles::loadAclConfig(const std::string &config_name, bool throw_exc)
{
	logAclManagerD() << "loadAclConfig:" << config_name;
	std::vector<std::string> files;
	if(config_name == FILE_ACL_MOUNTS)
		files = std::vector<std::string>{FILE_ACL_MOUNTS, FILE_ACL_MOUNTS_DEPR};
	else if(config_name == FILE_ACL_USERS)
		files = std::vector<std::string>{FILE_ACL_USERS};
	else if(config_name == FILE_ACL_ROLES)
		files = std::vector<std::string>{FILE_ACL_ROLES, FILE_ACL_ROLES_DEPR};
	else if(config_name == FILE_ACL_ACCESS)
		files = std::vector<std::string>{FILE_ACL_ACCESS, FILE_ACL_ACCESS_DEPR};
	std::ifstream fis;
	for(auto fn : files) {
		if(fn[0] != '/')
			fn = configDir() + '/' + fn;
		fis.open(fn);
		if (fis.good())
			break;
	}
	if (!fis.good()) {
		if(throw_exc)
			throw std::runtime_error("Cannot open config file " + config_name + " for reading");
		else
			return cp::RpcValue();
	}
	shv::chainpack::CponReader rd(fis);
	shv::chainpack::RpcValue rv;
	std::string err;
	rv = rd.read(throw_exc? nullptr: &err);
	return rv;
}

std::vector<std::string> AclManagerConfigFiles::aclMountDeviceIds()
{
	const chainpack::RpcValue cfg = aclConfig(FILE_ACL_MOUNTS);
	return cp::Utils::mapKeys(cfg.toMap());
}

AclMountDef AclManagerConfigFiles::aclMountDef(const std::string &device_id)
{
	chainpack::RpcValue v = aclConfig(FILE_ACL_MOUNTS).toMap().value(device_id);
	return AclMountDef::fromRpcValue(v);
}

std::vector<std::string> AclManagerConfigFiles::aclUsers()
{
	const chainpack::RpcValue cfg = aclConfig(FILE_ACL_USERS);
	shvDebug() << cfg.toCpon("\t");
	return cp::Utils::mapKeys(cfg.toMap());
}

AclUser AclManagerConfigFiles::aclUser(const std::string &user_name)
{
	chainpack::RpcValue v = aclConfig(FILE_ACL_USERS).toMap().value(user_name);
	return AclUser::fromRpcValue(v);
}

std::vector<std::string> AclManagerConfigFiles::aclRoles()
{
	const chainpack::RpcValue cfg = aclConfig(FILE_ACL_ROLES);
	return cp::Utils::mapKeys(cfg.toMap());
}

AclRole AclManagerConfigFiles::aclRole(const std::string &role_name)
{
	chainpack::RpcValue v = aclConfig(FILE_ACL_ROLES).toMap().value(role_name);
	return AclRole::fromRpcValue(v);
}

std::vector<std::string> AclManagerConfigFiles::aclAccessRoles()
{
	const chainpack::RpcValue cfg = aclConfig(FILE_ACL_ACCESS);
	return cp::Utils::mapKeys(cfg.toMap());
}

AclRoleAccessRules AclManagerConfigFiles::aclAccessRoleRules(const std::string &role_name)
{
	chainpack::RpcValue v = aclConfig(FILE_ACL_ACCESS).toMap().value(role_name);
	return AclRoleAccessRules::fromRpcValue(v);
}

} // namespace broker
} // namespace shv
