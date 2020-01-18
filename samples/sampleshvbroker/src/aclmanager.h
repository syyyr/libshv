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
	shv::chainpack::AclMountDef aclMountDef(const std::string &device_id) override { Q_UNUSED(device_id) return shv::chainpack::AclMountDef(); }
	std::vector<std::string> aclUsers() override;
	shv::chainpack::AclUser aclUser(const std::string &user_name) override;
	std::vector<std::string> aclRoles() override;
	shv::chainpack::AclRole aclRole(const std::string &role_name) override;
	std::vector<std::string> aclPathsRoles() override;
	shv::chainpack::AclRolePaths aclPathsRolePaths(const std::string &role_name) override;
};

#endif // ACLMANAGER_H
