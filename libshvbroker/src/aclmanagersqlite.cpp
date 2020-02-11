#include "aclmanagersqlite.h"
#include "brokerapp.h"

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDriver>
#include <QSqlRecord>

#define logAclManagerD() nCDebug("AclManager")
#define logAclManagerM() nCMessage("AclManager")
#define logAclManagerI() nCInfo("AclManager")

namespace cp = shv::chainpack;

namespace shv {
namespace broker {

const auto TBL_ACL_FSTAB = QStringLiteral("acl_fstab");
const auto TBL_ACL_USERS = QStringLiteral("acl_users");
const auto TBL_ACL_ROLES = QStringLiteral("acl_roles");
const auto TBL_ACL_PATHS = QStringLiteral("acl_paths");

AclManagerSqlite::AclManagerSqlite(BrokerApp *broker_app)
	: Super(broker_app)
{
	shvInfo() << "Creating Sqlite ACL Manager";
	checkAclTables();
}

AclManagerSqlite::~AclManagerSqlite()
{
}

QSqlQuery AclManagerSqlite::execSql(const QString &query_str)
{
	QSqlDatabase db = m_brokerApp->sqlConfigConnection();
	QSqlQuery q(db);
	if(!q.exec(query_str))
		SHV_EXCEPTION("SQL ERROR: " + q.lastError().text().toStdString() + "\nQuery: " + query_str.toStdString());
	return q;
}

std::vector<std::string> AclManagerSqlite::sqlLoadFields(const QString &table, const QString &column)
{
	std::vector<std::string> ret;
	QSqlQuery q = execSql("SELECT " + column + " FROM " + table + " ORDER BY " + column);
	while (q.next()) {
		ret.push_back(q.value(0).toString().toStdString());
	}
	return ret;
}

QSqlQuery AclManagerSqlite::sqlLoadRow(const QString &table, const QString &key_name, const QString &key_value)
{
	return execSql("SELECT * FROM " + table + " WHERE " + key_name + "='" + key_value + "'");
}

void AclManagerSqlite::checkAclTables()
{
	bool db_exist = true;
	try {
		execSql("SELECT COUNT(*) FROM " + TBL_ACL_USERS);
	}
	catch (const shv::core::Exception &) {
		shvInfo() << "Table 'users' does not exist.";
		db_exist = false;
	}
	if(!db_exist) {
		execSql("BEGIN TRANSACTION");
		createAclSqlTables();
		importAclConfigFiles();
		execSql("COMMIT");
	}
}

void AclManagerSqlite::createAclSqlTables()
{
	shvInfo() << "Creating SQLite ACL tables";
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
				deviceId character varying PRIMARY KEY,
				mountPoint character varying,
				description character varying
			);
			)kkt").arg(TBL_ACL_FSTAB));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
				name character varying PRIMARY KEY,
				password character varying,
				passwordFormat character varying,
				roles character varying
			);
			)kkt").arg(TBL_ACL_USERS));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
				name character varying PRIMARY KEY,
				weight integer,
				roles character varying
			);
			)kkt").arg(TBL_ACL_ROLES));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
				role character varying,
				path character varying,
				-- forwardGrant boolean,
				grantType character varying,
				accessLevel integer,
				accessRole character varying,
				user character varying,
				password character varying,
				loginType character varying,
				PRIMARY KEY (role, path)
			);
			)kkt").arg(TBL_ACL_PATHS));
}

static QString join_str_vec(const std::vector<std::string> &lst)
{
	QStringList qlst;
	for(const std::string &s : lst)
		qlst << QString::fromStdString(s);
	return qlst.join(',');
}

static std::vector<std::string> split_str_vec(const QString &ss)
{
	std::vector<std::string> ret;
	for(const QString &s : ss.split(',', QString::SkipEmptyParts))
		ret.push_back(s.trimmed().toStdString());
	return ret;
}

void AclManagerSqlite::importAclConfigFiles()
{
	AclManagerConfigFiles facl(m_brokerApp);
	shvInfo() << "Importing ACL config files from:" << facl.configDir();
	for(std::string id : facl.mountDeviceIds()) {
		AclMountDef md = facl.mountDef(id);
		//logAclManagerD() << id << md.toRpcValueMap().toCpon();
		if(!md.isValid())
			shvWarning() << "Cannot import invalid mount definition for device id:" << id;
		else
			aclSetMountDef(id, md);
	}
	for(std::string user : facl.users()) {
		AclUser u = facl.user(user);
		aclSetUser(user, u);
	}
	for(std::string role : facl.roles()) {
		AclRole r = facl.role(role);
		aclSetRole(role, r);
	}
	for(std::string role : facl.pathsRoles()) {
		AclRolePaths rpt = facl.pathsRolePaths(role);
		if(!rpt.isValid())
			shvWarning() << "Cannot import invalid role paths definition for role:" << role;
		else
			aclSetRolePaths(role, rpt);
	}
}

std::vector<std::string> AclManagerSqlite::aclMountDeviceIds()
{
	return sqlLoadFields(TBL_ACL_FSTAB, "deviceId");
}

AclMountDef AclManagerSqlite::aclMountDef(const std::string &device_id)
{
	AclMountDef ret;
	QSqlQuery q = sqlLoadRow(TBL_ACL_FSTAB, "deviceId", QString::fromStdString(device_id));
	if(q.next()) {
		//ret.deviceId = device_id;
		ret.mountPoint = q.value("mountPoint").toString().toStdString();
		ret.description = q.value("description").toString().toStdString();
	}
	return ret;
}

void AclManagerSqlite::aclSetMountDef(const std::string &device_id, const AclMountDef &md)
{
	if(md.isValid()) {
		QString qs = "INSERT OR REPLACE INTO " + TBL_ACL_FSTAB + " (deviceId, mountPoint, description) VALUES('%1', '%2', '%3')";
		qs = qs.arg(QString::fromStdString(device_id));
		qs = qs.arg(QString::fromStdString(md.mountPoint));
		qs = qs.arg(QString::fromStdString(md.description));
		logAclManagerM() << qs;
		execSql(qs);
	}
	else {
		QString qs = "DELETE FROM " + TBL_ACL_FSTAB + " WHERE deviceId='" + QString::fromStdString(device_id) + "'";
		logAclManagerM() << qs;
		execSql(qs);
	}
}

std::vector<std::string> AclManagerSqlite::aclUsers()
{
	return sqlLoadFields(TBL_ACL_USERS, "name");
}

AclUser AclManagerSqlite::aclUser(const std::string &user_name)
{
	AclUser ret;
	QSqlQuery q = sqlLoadRow(TBL_ACL_USERS, "name", QString::fromStdString(user_name));
	if(q.next()) {
		//ret.name = user_name;
		ret.password.password = q.value("password").toString().toStdString();
		ret.password.format = AclPassword::formatFromString(q.value("passwordFormat").toString().toStdString());
		ret.roles = split_str_vec(q.value("roles").toString());
	}
	return ret;
}

void AclManagerSqlite::aclSetUser(const std::string &user_name, const AclUser &u)
{
	if(u.isValid()) {
		QString qs = "INSERT OR REPLACE INTO " + TBL_ACL_USERS + " (name, password, passwordFormat, roles) VALUES('%1', '%2', '%3', '%4')";
		qs = qs.arg(QString::fromStdString(user_name));
		qs = qs.arg(QString::fromStdString(u.password.password));
		qs = qs.arg(QString::fromStdString(AclPassword::formatToString(u.password.format)));
		qs = qs.arg(join_str_vec(u.roles));
		logAclManagerM() << qs;
		execSql(qs);
	}
	else {
		QString qs = "DELETE FROM " + TBL_ACL_USERS + " WHERE name='" + QString::fromStdString(user_name) + "'";
		logAclManagerM() << qs;
		execSql(qs);
	}
}

std::vector<std::string> AclManagerSqlite::aclRoles()
{
	return sqlLoadFields(TBL_ACL_ROLES, "name");
}

AclRole AclManagerSqlite::aclRole(const std::string &role_name)
{
	AclRole ret;
	QSqlQuery q = sqlLoadRow(TBL_ACL_ROLES, "name", QString::fromStdString(role_name));
	if(q.next()) {
		//ret.name = user_name;
		ret.weight = q.value("weight").toInt();
		ret.roles = split_str_vec(q.value("roles").toString());
	}
	return ret;
}

void AclManagerSqlite::aclSetRole(const std::string &role_name, const AclRole &r)
{
	if(r.isValid()) {
		QString qs = "INSERT OR REPLACE INTO " + TBL_ACL_ROLES + " (name, weight, roles) VALUES('%1', %2, '%3')";
		qs = qs.arg(QString::fromStdString(role_name));
		qs = qs.arg(r.weight);
		qs = qs.arg(join_str_vec(r.roles));
		logAclManagerM() << qs;
		execSql(qs);
	}
	else {
		execSql("DELETE FROM " + TBL_ACL_ROLES + " WHERE name='" + QString::fromStdString(role_name) + "'");
	}
}

std::vector<std::string> AclManagerSqlite::aclPathsRoles()
{
	std::vector<std::string> ret;
	QSqlQuery q = execSql("SELECT role FROM " + TBL_ACL_PATHS + " GROUP BY role ORDER BY role");
	while (q.next()) {
		ret.push_back(q.value(0).toString().toStdString());
	}
	return ret;
}

AclRolePaths AclManagerSqlite::aclPathsRolePaths(const std::string &role_name)
{
	AclRolePaths ret;
	QSqlQuery q = execSql("SELECT * FROM " + TBL_ACL_PATHS + " WHERE role='" + QString::fromStdString(role_name) + "'");
	while(q.next()) {
		std::string path = q.value("path").toString().toStdString();
		cp::PathAccessGrant ag;
		//ag.forwardUserLoginFromRequest = q.value(cp::PathAccessGrant::FORWARD_USER_LOGIN).toBool();
		std::string grant_type = q.value("grantType").toString().toStdString();
		ag.type = cp::PathAccessGrant::typeFromString(grant_type);
		switch (ag.type) {
		case cp::PathAccessGrant::Type::AccessLevel: ag.accessLevel = q.value("accessLevel").toInt(); break;
		case cp::PathAccessGrant::Type::Role: ag.role = q.value("accessRole").toString().toStdString(); break;
		case cp::PathAccessGrant::Type::UserLogin: {
			ag.login.user = q.value("user").toString().toStdString();
			ag.login.password = q.value("password").toString().toStdString();
			ag.login.loginType = cp::UserLogin::loginTypeFromString(q.value("loginType").toString().toStdString());
			break;
		}
		default:
			SHV_EXCEPTION("Invalid PathAccessGrant type: " + grant_type);
		}
		ret[path] = std::move(ag);
	}
	return ret;
}

void AclManagerSqlite::aclSetRolePaths(const std::string &role_name, const AclRolePaths &rpt)
{
	QString qs = "DELETE FROM " + TBL_ACL_PATHS + " WHERE role='" + QString::fromStdString(role_name) + "'";
	logAclManagerM() << qs;
	execSql(qs);
	if(rpt.isValid()) {
		QSqlDatabase db = m_brokerApp->sqlConfigConnection();
		QSqlDriver *drv = db.driver();
		QSqlRecord rec = drv->record(TBL_ACL_PATHS);
		for(const auto &kv : rpt) {
			rec.setValue("role", QString::fromStdString(role_name));
			rec.setValue("path", QString::fromStdString(kv.first));
			const cp::PathAccessGrant &g = kv.second;
			//if(g.forwardUserLoginFromRequest)
			//	rec.setValue(cp::PathAccessGrant::FORWARD_USER_LOGIN, g.forwardUserLoginFromRequest);
			rec.setValue("grantType", cp::PathAccessGrant::typeToString(g.type));
			switch (g.type) {
			case cp::PathAccessGrant::Type::AccessLevel: rec.setValue("accessLevel", g.accessLevel); break;
			case cp::PathAccessGrant::Type::Role: rec.setValue("accessRole", QString::fromStdString(g.role)); break;
			case cp::PathAccessGrant::Type::UserLogin: {
				rec.setValue("user", QString::fromStdString(g.login.user));
				rec.setValue("password", QString::fromStdString(g.login.password));
				rec.setValue("loginType", cp::UserLogin::loginTypeToString(g.login.loginType));
				break;
			}
			default:
				SHV_EXCEPTION("Invalid PathAccessGrant type: " + std::to_string((int)g.type));
			}
			qs = drv->sqlStatement(QSqlDriver::InsertStatement, TBL_ACL_PATHS, rec, false);
			logAclManagerM() << qs;
			execSql(qs);
		}
	}
}

} // namespace broker
} // namespace shv
