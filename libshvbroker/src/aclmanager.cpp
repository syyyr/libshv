#include "aclmanager.h"
#include "brokerapp.h"

#include <shv/chainpack/cponreader.h>
#include <shv/coreqt/log.h>

#include <QCryptographicHash>

#include <fstream>

namespace cp = shv::chainpack;

namespace shv {
namespace broker {
namespace {
static std::string sha1_hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
	hash.addData(s.data(), s.length());
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
	if(m_aclMountDefs.empty()) {
		for(auto id : aclMountDeviceIds())
			m_aclMountDefs[id];
	}
	return cp::Utils::mapKeys(m_aclMountDefs);
}

chainpack::AclMountDef AclManager::mountDef(const std::string &device_id)
{
	if(m_aclMountDefs.empty())
		mountDeviceIds();
	auto it = m_aclMountDefs.find(device_id);
	if(it == m_aclMountDefs.end())
		return chainpack::AclMountDef();
	if(!it->second.isValid()) {
		it->second = aclMountDef(device_id);
	}
	return it->second;
}

std::vector<std::string> AclManager::users()
{
	if(m_aclUsers.empty()) {
		for(auto id : aclUsers())
			m_aclUsers[id];
	}
	return cp::Utils::mapKeys(m_aclUsers);
}

chainpack::AclUser AclManager::user(const std::string &user_name)
{
	if(m_aclUsers.empty())
		users();
	auto it = m_aclUsers.find(user_name);
	if(it == m_aclUsers.end())
		return chainpack::AclUser();
	if(!it->second.isValid()) {
		it->second = aclUser(user_name);
	}
	return it->second;
}

std::vector<std::string> AclManager::roles()
{
	if(m_aclRoles.empty()) {
		for(auto id : aclRoles())
			m_aclRoles[id];
	}
	return cp::Utils::mapKeys(m_aclRoles);
}

chainpack::AclRole AclManager::role(const std::string &role_name)
{
	if(m_aclRoles.empty())
		roles();
	auto it = m_aclRoles.find(role_name);
	if(it == m_aclRoles.end())
		return chainpack::AclRole();
	if(!it->second.isValid()) {
		it->second = aclRole(role_name);
	}
	return it->second;
}

std::vector<std::string> AclManager::pathsRoles()
{
	if(m_aclPathsRoles.empty()) {
		for(auto id : aclPathsRoles())
			m_aclPathsRoles[id];
	}
	return cp::Utils::mapKeys(m_aclPathsRoles);
}

chainpack::AclRolePaths AclManager::pathsRolePaths(const std::string &role_name)
{
	if(m_aclPathsRoles.empty())
		pathsRoles();
	auto it = m_aclPathsRoles.find(role_name);
	if(it == m_aclPathsRoles.end())
		return chainpack::AclRolePaths();
	if(!it->second.isValid()) {
		it->second = aclPathsRolePaths(role_name);
	}
	return it->second;
}

bool AclManager::checkPassword(const chainpack::UserLogin &login, const chainpack::UserLoginContext &login_context)
{
	using LoginType = cp::UserLogin::LoginType;
	chainpack::AclUser acl_user = user(login.user);
	if(!acl_user.isValid()) {
		shvError() << "Invalid user name:" << login.user;
		return false;
	}
	auto acl_pwd = acl_user.password;
	//std::string pwdsrv = std::get<0>(pwdt);
	//PasswordFormat pwdsrvfmt = std::get<1>(pwdt);
	if(acl_pwd.password.empty()) {
		shvError() << "Empty password not allowed:" << acl_user.name;
		return false;
	}
	auto usr_login_type = login.loginType;
	if(usr_login_type == LoginType::Invalid)
		usr_login_type = LoginType::Sha1;

	std::string usr_pwd = login.password;
	if(usr_login_type == LoginType::Plain) {
		if(acl_pwd.format == cp::AclPassword::Format::Plain)
			return (acl_pwd.password == usr_pwd);
		if(acl_pwd.format == cp::AclPassword::Format::Sha1)
			return acl_pwd.password == sha1_hex(usr_pwd);
	}
	if(usr_login_type == LoginType::Sha1) {
		/// login_type == "SHA1" is default
		if(acl_pwd.format == cp::AclPassword::Format::Plain)
			acl_pwd.password = sha1_hex(acl_pwd.password);

		std::string nonce = login_context.serverNounce + acl_pwd.password;
		//shvWarning() << m_pendingAuthNonce << "prd" << nonce;
		QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
		hash.addData(nonce.data(), nonce.length());
		std::string correct_sha1 = std::string(hash.result().toHex().constData());
		//shvInfo() << nonce_sha1 << "vs" << sha1;
		return (usr_pwd == correct_sha1);
	}
	{
		shvError() << "Unsupported login type" << cp::UserLogin::loginTypeToString(login.loginType) << "for user:" << login.user;
		return false;
	}
}

std::string AclManager::mountPointForDevice(const chainpack::RpcValue &device_id)
{
	return mountDef(device_id.toString()).mountPoint;
}

void AclManager::reload()
{
	clearCache();
}

std::set<std::string> AclManager::flattenRole_helper(const std::string &role_name)
{
	std::set<std::string> ret;
	chainpack::AclRole ar = aclRole(role_name);
	if(ar.isValid()) {
		ret.insert(ar.name);
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
	if(m_userFlattenRoles.find(user_name) == m_userFlattenRoles.end()) {
		std::set<std::string> unique_roles;
		chainpack::AclUser user_def = aclUser(user_name);
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
		m_userFlattenRoles[user_name] = roles;
	}
	return m_userFlattenRoles[user_name];
}

//================================================================
// AclManagerConfigFiles
//================================================================

AclManagerConfigFiles::AclManagerConfigFiles(BrokerApp *broker_app)
	: Super(broker_app)
{

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
	std::string fn = config_name;
	fn = m_brokerApp->cliOptions()->value("etc.acl." + fn).toString();
	if(fn.empty()) {
		if(throw_exc)
			throw std::runtime_error("config file name is empty.");
		else
			return cp::RpcValue();
	}
	if(fn[0] != '/')
		fn = m_brokerApp->cliOptions()->configDir() + '/' + fn;
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
/*
bool BrokerApp::saveAclConfig(const std::string &config_name, const shv::chainpack::RpcValue &config, bool throw_exc)
{
	logAclD() << "saveAclConfig" << config_name << "config type:" << config.typeToName(config.type());
	//logAclD() << "config:" << config.toCpon();
	std::string fn = config_name;
	fn = cliOptions()->value("etc.acl." + fn).toString();
	if(fn.empty()) {
		if(throw_exc)
			throw std::runtime_error("config file name is empty.");
		else
			return false;
	}
	if(fn[0] != '/')
		fn = cliOptions()->configDir() + '/' + fn;

	if(config.isMap()) {
		std::ofstream fos(fn, std::ios::binary | std::ios::trunc);
		if (!fos) {
			if(throw_exc)
				throw std::runtime_error("Cannot open config file " + fn + " for writing");
			else
				return false;
		}
		shv::chainpack::CponWriterOptions opts;
		opts.setIndent("  ");
		shv::chainpack::CponWriter wr(fos, opts);
		wr << config;
		return true;
	}
	else {
		if(throw_exc)
			throw std::runtime_error("Config must be RpcValue::Map type, config name: " + config_name);
		else
			return false;
	}
}
*/

} // namespace broker
} // namespace shv
