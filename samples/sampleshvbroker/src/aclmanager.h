#ifndef ACLMANAGER_H
#define ACLMANAGER_H

#include <shv/broker/aclmanager.h>

class AclManager : public shv::broker::AclManager
{
	using Super = shv::broker::AclManager;
public:
	AclManager(shv::broker::BrokerApp *broker_app);

	// AclManager interface
protected:
	std::vector<std::string> aclMountDeviceIds() override { return std::vector<std::string>(); }
	shv::broker::AclMountDef aclMountDef(const std::string &device_id) override { Q_UNUSED(device_id) return shv::broker::AclMountDef(); }
	std::vector<std::string> aclUsers() override;
	shv::broker::AclUser aclUser(const std::string &user_name) override;
	std::vector<std::string> aclRoles() override;
	shv::broker::AclRole aclRole(const std::string &role_name) override;
	std::vector<std::string> aclAccessRoles() override;
	shv::broker::AclRoleAccessRules aclAccessRoleRules(const std::string &role_name) override;
};

#endif // ACLMANAGER_H
