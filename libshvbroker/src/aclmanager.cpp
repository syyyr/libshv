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

std::vector<std::string> AclManager::pathsRoles()
{
	if(m_cache.aclPathsRoles.empty()) {
		for(auto id : aclPathsRoles())
			m_cache.aclPathsRoles[id];
	}
	return cp::Utils::mapKeys(m_cache.aclPathsRoles);
}

AclRolePaths AclManager::pathsRolePaths(const std::string &role_name)
{
	if(m_cache.aclPathsRoles.empty())
		pathsRoles();
	auto it = m_cache.aclPathsRoles.find(role_name);
	if(it == m_cache.aclPathsRoles.end())
		return AclRolePaths();
	if(!it->second.isValid()) {
		it->second = aclPathsRolePaths(role_name);
	}
	return it->second;
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

void AclManager::aclSetRolePaths(const std::string &role_name, const AclRolePaths &rp)
{
	Q_UNUSED(role_name)
	Q_UNUSED(rp)
	SHV_EXCEPTION("Role paths definition is read only.");
}

std::set<std::string> AclManager::flattenRole_helper(const std::string &role_name)
{
	std::set<std::string> ret;
	AclRole ar = aclRole(role_name);
	if(ar.isValid()) {
		ret.insert(role_name);
		for(auto g : ar.roles) {
			if(ret.count(g)) {
				shvWarning() << "Cyclic reference in grants detected for grant name:" << g;
			}
			else {
				std::set<std::string> gg = flattenRole_helper(g);
				ret.insert(gg.begin(), gg.end());
			}
		}
	}
	return ret;
}

std::vector<std::string> AclManager::userFlattenRolesSortedByWeight(const std::string &user_name)
{
	if(m_cache.userFlattenRoles.find(user_name) == m_cache.userFlattenRoles.end()) {
		std::set<std::string> unique_roles;
		AclUser user_def = aclUser(user_name);
		if(!user_def.isValid())
			return std::vector<std::string>();

		for(auto role : user_def.roles) {
			std::set<std::string> gg = flattenRole_helper(role);
			unique_roles.insert(gg.begin(), gg.end());
		}
		std::vector<std::string> roles;
		for(auto s : unique_roles)
			roles.push_back(s);
		std::sort(roles.begin(), roles.end(), [this](const std::string &a, const std::string &b) {
			return role(a).weight > role(b).weight;
		});
		m_cache.userFlattenRoles[user_name] = roles;
	}
	return m_cache.userFlattenRoles[user_name];
}

//================================================================
// AclManagerConfigFiles
//================================================================

AclManagerConfigFiles::AclManagerConfigFiles(BrokerApp *broker_app)
	: Super(broker_app)
{
	shvInfo() << "Creating AclManagerConfigFiles";
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
	std::string fn = config_name;
	fn = config_name + ".cpon"; //m_brokerApp->cliOptions()->value("etc.acl." + fn).toString();
	if(fn[0] != '/')
		fn = configDir() + '/' + fn;
	std::ifstream fis(fn);
	if (!fis.good()) {
		if(throw_exc)
			throw std::runtime_error("Cannot open config file " + fn + " for reading");
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
	const chainpack::RpcValue cfg = aclConfig("fstab");
	return cp::Utils::mapKeys(cfg.toMap());
}

AclMountDef AclManagerConfigFiles::aclMountDef(const std::string &device_id)
{
	chainpack::RpcValue v = aclConfig("fstab").toMap().value(device_id);
	return AclMountDef::fromRpcValue(v);
}

std::vector<std::string> AclManagerConfigFiles::aclUsers()
{
	const chainpack::RpcValue cfg = aclConfig("users");
	shvDebug() << cfg.toCpon("\t");
	return cp::Utils::mapKeys(cfg.toMap());
}

AclUser AclManagerConfigFiles::aclUser(const std::string &user_name)
{
	chainpack::RpcValue v = aclConfig("users").toMap().value(user_name);
	return AclUser::fromRpcValue(v);
}

std::vector<std::string> AclManagerConfigFiles::aclRoles()
{
	const chainpack::RpcValue cfg = aclConfig("grants");
	return cp::Utils::mapKeys(cfg.toMap());
}

AclRole AclManagerConfigFiles::aclRole(const std::string &role_name)
{
	chainpack::RpcValue v = aclConfig("grants").toMap().value(role_name);
	return AclRole::fromRpcValue(v);
}

std::vector<std::string> AclManagerConfigFiles::aclPathsRoles()
{
	const chainpack::RpcValue cfg = aclConfig("paths");
	return cp::Utils::mapKeys(cfg.toMap());
}

AclRolePaths AclManagerConfigFiles::aclPathsRolePaths(const std::string &role_name)
{
	chainpack::RpcValue v = aclConfig("paths").toMap().value(role_name);
	return AclRolePaths::fromRpcValue(v);
}

} // namespace broker
} // namespace shv
