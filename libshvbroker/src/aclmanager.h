#pragma once

#include "shvbrokerglobal.h"

#include "aclrole.h"
#include "aclrolepaths.h"
#include "aclmountdef.h"
#include "acluser.h"
#include "aclpassword.h"
#include <shv/core/exception.h>

#include <QObject>

#include <string>
#include <set>

#define ACCESS_EXCEPTION(msg) SHV_EXCEPTION_V(msg, "Access")

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
	AclRolePaths accessRolePaths(const std::string &role_name);
	void setAccessRolePaths(const std::string &role_name, const AclRolePaths &v);

	std::string mountPointForDevice(const shv::chainpack::RpcValue &device_id);

	struct SHVBROKER_DECL_EXPORT FlattenRole
	{
		std::string name;
		int weight = 0;
		int nestLevel = 0;

		FlattenRole(const std::string &n, int w = 0, int nl = 0) : name(n), weight(w), nestLevel(nl) {}
		bool operator<(const FlattenRole &o) const {
			if(weight == o.weight)
				return nestLevel < o.nestLevel;
			return weight > o.weight;
		}
	};
	std::set<FlattenRole> userFlattenRoles(const std::string &user_name);

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
	virtual AclRolePaths aclAccessRolePaths(const std::string &role_name) = 0;
	virtual void aclSetAccessRolePaths(const std::string &role_name, const AclRolePaths &rp);

	//virtual AclPassword aclUserPassword(const std::string &user) = 0;
protected:
	virtual void clearCache()
	{
		m_cache = Cache();
	}
	std::set<FlattenRole> flattenRole_helper(const std::string &role_name, int nest_level);
protected:
	BrokerApp * m_brokerApp;
	struct Cache
	{
		std::map<std::string, AclMountDef> aclMountDefs;
		std::map<std::string, AclUser> aclUsers;
		std::map<std::string, AclRole> aclRoles;
		std::map<std::string, AclRolePaths> aclPathsRoles;

		std::map<std::string, std::set<FlattenRole>> userFlattenRoles;
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
	AclRolePaths aclAccessRolePaths(const std::string &role_name) override;
protected:
	shv::chainpack::RpcValue::Map m_configFiles;
};

} // namespace broker
} // namespace shv

