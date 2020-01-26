#ifndef SHV_BROKER_ACLMANAGERSQLITE_H
#define SHV_BROKER_ACLMANAGERSQLITE_H

#include "aclmanager.h"

class QSqlQuery;

namespace shv {
namespace broker {

class SHVBROKER_DECL_EXPORT AclManagerSqlite : public AclManager
{
	Q_OBJECT
	using Super = AclManager;
public:
	AclManagerSqlite(BrokerApp *broker_app);
	~AclManagerSqlite() override;

	// AclManager interface
protected:
	std::vector<std::string> aclMountDeviceIds() override;
	chainpack::AclMountDef aclMountDef(const std::string &device_id) override;
	void aclSetMountDef(const std::string &device_id, const shv::chainpack::AclMountDef &md) override;

	std::vector<std::string> aclUsers() override;
	chainpack::AclUser aclUser(const std::string &user_name) override;
	void aclSetUser(const std::string &user_name, const shv::chainpack::AclUser &u) override;

	std::vector<std::string> aclRoles() override;
	chainpack::AclRole aclRole(const std::string &role_name) override;
	void aclSetRole(const std::string &role_name, const shv::chainpack::AclRole &r) override;

	std::vector<std::string> aclPathsRoles() override;
	chainpack::AclRolePaths aclPathsRolePaths(const std::string &role_name) override;
	void aclSetRolePaths(const std::string &role_name, const shv::chainpack::AclRolePaths &rpt) override;
private:
	void initDbConnection();
	void createSqliteDatabase(const QString &file_name);
	void importFileAclConfig();

	QSqlQuery execSql(const QString &query_str);
	std::vector<std::string> sqlLoadFields(const QString &table, const QString &column);
	QSqlQuery sqlLoadRow(const QString &table, const QString &key_name, const QString &key_value);
};

} // namespace broker
} // namespace shv

#endif // SHV_BROKER_ACLMANAGERSQLITE_H
