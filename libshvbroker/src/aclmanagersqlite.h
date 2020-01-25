#ifndef SHV_BROKER_ACLMANAGERSQLITE_H
#define SHV_BROKER_ACLMANAGERSQLITE_H

#include "aclmanager.h"

namespace shv {
namespace broker {

class SHVBROKER_DECL_EXPORT AclManagerSqlite : public AclManager
{
	using Super = AclManager;
public:
	AclManagerSqlite(BrokerApp *broker_app);
	~AclManagerSqlite() override;

	// AclManager interface
protected:
	std::vector<std::string> aclMountDeviceIds() override;
	chainpack::AclMountDef aclMountDef(const std::string &device_id) override;
	std::vector<std::string> aclUsers() override;
	chainpack::AclUser aclUser(const std::string &user_name) override;
	std::vector<std::string> aclRoles() override;
	chainpack::AclRole aclRole(const std::string &role_name) override;
	std::vector<std::string> aclPathsRoles() override;
	chainpack::AclRolePaths aclPathsRolePaths(const std::string &role_name) override;
};

} // namespace broker
} // namespace shv

#endif // SHV_BROKER_ACLMANAGERSQLITE_H
