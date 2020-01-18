#pragma once

#include "shvbrokerglobal.h"

#include <shv/chainpack/aclrole.h>
#include <shv/chainpack/aclrolepaths.h>
#include <shv/chainpack/aclmountdef.h>
#include <shv/chainpack/acluser.h>
#include <shv/chainpack/aclpassword.h>

#include <QObject>

#include <string>
#include <set>

namespace shv {
namespace broker {

class BrokerApp;

class SHVBROKER_DECL_EXPORT AclManager : public QObject
{
	Q_OBJECT
public:
	AclManager(BrokerApp *broker_app);

	std::vector<std::string> mountDeviceIds();
	shv::chainpack::AclMountDef mountDef(const std::string &device_id);

	std::vector<std::string> users();
	shv::chainpack::AclUser user(const std::string &user_name);

	std::vector<std::string> roles();
	shv::chainpack::AclRole role(const std::string &role_name);

	std::vector<std::string> pathsRoles();
	shv::chainpack::AclRolePaths pathsRolePaths(const std::string &role_name);

	virtual bool checkPassword(const shv::chainpack::UserLogin &login, const shv::chainpack::UserLoginContext &login_context);

	std::string mountPointForDevice(const shv::chainpack::RpcValue &device_id);
	std::vector<std::string> userFlattenRolesSortedByWeight(const std::string &user_name);

	virtual void reload();
protected:
	virtual std::vector<std::string> aclMountDeviceIds() = 0;
	virtual shv::chainpack::AclMountDef aclMountDef(const std::string &device_id) = 0;

	virtual std::vector<std::string> aclUsers() = 0;
	virtual shv::chainpack::AclUser aclUser(const std::string &user_name) = 0;

	virtual std::vector<std::string> aclRoles() = 0;
	virtual shv::chainpack::AclRole aclRole(const std::string &role_name) = 0;

	virtual std::vector<std::string> aclPathsRoles() = 0;
	virtual shv::chainpack::AclRolePaths aclPathsRolePaths(const std::string &role_name) = 0;

	//virtual shv::chainpack::AclPassword aclUserPassword(const std::string &user) = 0;
protected:
	virtual void clearCache()
	{
		m_aclMountDefs.clear();
		m_aclUsers.clear();
		m_userFlattenRoles.clear();
		m_aclRoles.clear();
		m_aclPathsRoles.clear();
	}
	std::set<std::string> flattenRole_helper(const std::string &role_name);
protected:
	BrokerApp * m_brokerApp;

	std::map<std::string, shv::chainpack::AclMountDef> m_aclMountDefs;
	std::map<std::string, shv::chainpack::AclUser> m_aclUsers;
	std::map<std::string, std::vector<std::string>> m_userFlattenRoles;
	std::map<std::string, shv::chainpack::AclRole> m_aclRoles;
	std::map<std::string, shv::chainpack::AclRolePaths> m_aclPathsRoles;
};

class SHVBROKER_DECL_EXPORT AclManagerConfigFiles : public AclManager
{
	using Super = AclManager;
public:
	AclManagerConfigFiles(BrokerApp *broker_app);

protected:
	void clearCache() override;
	shv::chainpack::RpcValue aclConfig(const std::string &config_name, bool throw_exc);
	shv::chainpack::RpcValue loadAclConfig(const std::string &config_name, bool throw_exc);
	//bool saveAclConfig(const std::string &config_name, const shv::chainpack::RpcValue &config, bool throw_exc);
protected:
	shv::chainpack::RpcValue::Map m_configFiles;
};

} // namespace broker
} // namespace shv

