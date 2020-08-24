#pragma once

#include "shvbrokerglobal.h"

#include "aclrole.h"
#include "aclroleaccessrules.h"
#include "aclmountdef.h"
#include "acluser.h"
#include "aclpassword.h"
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
	AclMountDef mountDef(const std::string &device_id);
	void setMountDef(const std::string &device_id, const AclMountDef &v);

	std::vector<std::string> users();
	AclUser user(const std::string &user_name);
	void setUser(const std::string &user_name, const AclUser &u);

	std::vector<std::string> roles();
	AclRole role(const std::string &role_name);
	void setRole(const std::string &role_name, const AclRole &v);

	std::vector<std::string> accessRoles();
	AclRoleAccessRules accessRoleRules(const std::string &role_name);
	void setAccessRoleRules(const std::string &role_name, const AclRoleAccessRules &v);

	std::string mountPointForDevice(const shv::chainpack::RpcValue &device_id);

	struct SHVBROKER_DECL_EXPORT FlattenRole
	{
		std::string name;
		int weight = 0;
		int nestLevel = 0;

		FlattenRole() {}
		FlattenRole(const std::string &n, int w = 0, int nl = 0) : name(n), weight(w), nestLevel(nl) {}
	};
	std::vector<FlattenRole> userFlattenRoles(const std::string &user_name);
	std::vector<FlattenRole> flattenRole(const std::string &role);

	virtual chainpack::UserLoginResult checkPassword(const chainpack::UserLoginContext &login_context);

	virtual void reload();
protected:
	virtual std::vector<std::string> aclMountDeviceIds() = 0;
	virtual AclMountDef aclMountDef(const std::string &device_id) = 0;
	virtual void aclSetMountDef(const std::string &device_id, const AclMountDef &md);

	virtual std::vector<std::string> aclUsers() = 0;
	virtual AclUser aclUser(const std::string &user_name) = 0;
	virtual void aclSetUser(const std::string &user_name, const AclUser &u);

	virtual std::vector<std::string> aclRoles() = 0;
	virtual AclRole aclRole(const std::string &role_name) = 0;
	virtual void aclSetRole(const std::string &role_name, const AclRole &r);

	virtual std::vector<std::string> aclAccessRoles() = 0;
	virtual AclRoleAccessRules aclAccessRoleRules(const std::string &role_name) = 0;
	virtual void aclSetAccessRoleRules(const std::string &role_name, const AclRoleAccessRules &rp);

	//virtual AclPassword aclUserPassword(const std::string &user) = 0;
protected:
	virtual void clearCache()
	{
		m_cache = Cache();
	}
	std::map<std::string, FlattenRole> flattenRole_helper(const std::string &role_name, int nest_level);
protected:
	BrokerApp * m_brokerApp;
	struct Cache
	{
		std::map<std::string, AclMountDef> aclMountDefs;
		std::map<std::string, AclUser> aclUsers;
		std::map<std::string, AclRole> aclRoles;
		std::map<std::string, std::pair<AclRoleAccessRules, bool>> aclPathsRoles;

		std::map<std::string, std::vector<FlattenRole>> userFlattenRoles;
	} m_cache;
};

class SHVBROKER_DECL_EXPORT AclManagerConfigFiles : public AclManager
{
	using Super = AclManager;
public:
	AclManagerConfigFiles(BrokerApp *broker_app);

	std::string configDir() const;
protected:
	void clearCache() override;
	shv::chainpack::RpcValue aclConfig(const std::string &config_name, bool throw_exc = shv::core::Exception::Throw);
	shv::chainpack::RpcValue loadAclConfig(const std::string &config_name, bool throw_exc);

	std::vector<std::string> aclMountDeviceIds() override;
	AclMountDef aclMountDef(const std::string &device_id) override;
	std::vector<std::string> aclUsers() override;
	AclUser aclUser(const std::string &user_name) override;
	std::vector<std::string> aclRoles() override;
	AclRole aclRole(const std::string &role_name) override;
	std::vector<std::string> aclAccessRoles() override;
	AclRoleAccessRules aclAccessRoleRules(const std::string &role_name) override;
protected:
	shv::chainpack::RpcValue::Map m_configFiles;
};

} // namespace broker
} // namespace shv

