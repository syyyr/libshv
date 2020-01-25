#pragma once

#include "shvbrokerglobal.h"

#include <shv/chainpack/aclrole.h>
#include <shv/chainpack/aclrolepaths.h>
#include <shv/chainpack/aclmountdef.h>
#include <shv/chainpack/acluser.h>
#include <shv/chainpack/aclpassword.h>
#include <shv/core/exception.h>

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
	void setUser(const std::string &user_name, const shv::chainpack::AclUser &u);

	std::vector<std::string> roles();
	shv::chainpack::AclRole role(const std::string &role_name);

	std::vector<std::string> pathsRoles();
	shv::chainpack::AclRolePaths pathsRolePaths(const std::string &role_name);

	std::string mountPointForDevice(const shv::chainpack::RpcValue &device_id);
	std::vector<std::string> userFlattenRolesSortedByWeight(const std::string &user_name);

	virtual void checkPassword(const chainpack::UserLoginContext &login_context);
	//Q_SIGNAL void loginResult(int client_id, const shv::chainpack::UserLoginResult &result);

	virtual void reload();
protected:
	virtual std::vector<std::string> aclMountDeviceIds() = 0;
	virtual shv::chainpack::AclMountDef aclMountDef(const std::string &device_id) = 0;
	virtual void aclSetMountDef(const std::string &device_id, const shv::chainpack::AclMountDef &md);

	virtual std::vector<std::string> aclUsers() = 0;
	virtual shv::chainpack::AclUser aclUser(const std::string &user_name) = 0;
	virtual void aclSetUser(const std::string &user_name, const shv::chainpack::AclUser &u);

	virtual std::vector<std::string> aclRoles() = 0;
	virtual shv::chainpack::AclRole aclRole(const std::string &role_name) = 0;

	virtual std::vector<std::string> aclPathsRoles() = 0;
	virtual shv::chainpack::AclRolePaths aclPathsRolePaths(const std::string &role_name) = 0;

	//virtual shv::chainpack::AclPassword aclUserPassword(const std::string &user) = 0;
protected:
	virtual void clearCache()
	{
		m_cache = Cache();
	}
	std::set<std::string> flattenRole_helper(const std::string &role_name);
protected:
	BrokerApp * m_brokerApp;
	struct Cache
	{
		std::map<std::string, shv::chainpack::AclMountDef> aclMountDefs;
		std::map<std::string, shv::chainpack::AclUser> aclUsers;
		std::map<std::string, shv::chainpack::AclRole> aclRoles;
		std::map<std::string, shv::chainpack::AclRolePaths> aclPathsRoles;

		std::map<std::string, std::vector<std::string>> userFlattenRoles;
	} m_cache;
};

class SHVBROKER_DECL_EXPORT AclManagerConfigFiles : public AclManager
{
	using Super = AclManager;
public:
	AclManagerConfigFiles(BrokerApp *broker_app);

protected:
	void clearCache() override;
	shv::chainpack::RpcValue aclConfig(const std::string &config_name, bool throw_exc = shv::core::Exception::Throw);
	shv::chainpack::RpcValue loadAclConfig(const std::string &config_name, bool throw_exc);

	std::vector<std::string> aclMountDeviceIds() override;
	shv::chainpack::AclMountDef aclMountDef(const std::string &device_id) override;
	std::vector<std::string> aclUsers() override;
	shv::chainpack::AclUser aclUser(const std::string &user_name) override;
	std::vector<std::string> aclRoles() override;
	shv::chainpack::AclRole aclRole(const std::string &role_name) override;
	std::vector<std::string> aclPathsRoles() override;
	shv::chainpack::AclRolePaths aclPathsRolePaths(const std::string &role_name) override;
protected:
	shv::chainpack::RpcValue::Map m_configFiles;
};

} // namespace broker
} // namespace shv

