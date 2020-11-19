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

const auto TBL_ACL_MOUNTS = QStringLiteral("acl_mounts");
const auto TBL_ACL_USERS = QStringLiteral("acl_users");
const auto TBL_ACL_ROLES = QStringLiteral("acl_roles");
const auto TBL_ACL_ACCESS = QStringLiteral("acl_access");

AclManagerSqlite::AclManagerSqlite(BrokerApp *broker_app)
	: Super(broker_app)
{
	shvInfo() << "Creating Sqlite ACL Manager";
	checkAclTables();
}

AclManagerSqlite::~AclManagerSqlite()
{
}

QSqlQuery AclManagerSqlite::execSql(const QString &query_str, bool throw_exc)
{
	QSqlDatabase db = m_brokerApp->sqlConfigConnection();
	QSqlQuery q(db);
	if(!q.exec(query_str) && throw_exc)
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
	{
		QSqlQuery q = execSql("SELECT COUNT(*) FROM " + TBL_ACL_USERS, !shv::core::Exception::Throw);
		bool db_exist = q.isActive();
		if(!db_exist) {
			shvInfo() << "Table" << TBL_ACL_USERS << "does not exist.";
			execSql("BEGIN TRANSACTION");
			createAclSqlTables();
			importAclConfigFiles();
			execSql("COMMIT");
		}
	}
	{
		auto col_name = QStringLiteral("profile");
		QSqlQuery q = execSql("SELECT " + col_name + " FROM " + TBL_ACL_ROLES + " WHERE name='xxx'", !shv::core::Exception::Throw);
		bool column_exist = q.isActive();
		if(!column_exist) {
			shvWarning() << "Table:" << TBL_ACL_ROLES << "column:" << col_name << "does not exist.";
			shvInfo() << "Adding column:" << TBL_ACL_ROLES << "::" << col_name;
			execSql("BEGIN TRANSACTION");
			execSql(QStringLiteral(R"kkt(
					ALTER TABLE %1 ADD profile varchar;
					)kkt").arg(TBL_ACL_ROLES));
			execSql("COMMIT");
		}
	}
	{
		bool column_method_exists = true;
		try {
			execSql("SELECT method FROM " + TBL_ACL_ACCESS);
		}
		catch (const shv::core::Exception &) {
			shvInfo().nospace() << "Column " << TBL_ACL_ACCESS << ".method does not exist.";
			column_method_exists = false;
		}
		if(!column_method_exists) {
			execSql("BEGIN TRANSACTION");
			createAclSqlTables();
			importAclConfigFiles();
			execSql("ALTER TABLE " + TBL_ACL_ACCESS + " ADD COLUMN method varchar");
		}
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
			)kkt").arg(TBL_ACL_MOUNTS));
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
				roles character varying,
				profile character varying
			);
			)kkt").arg(TBL_ACL_ROLES));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
				role character varying,
				path character varying,
				method character varying,
				grantType character varying,
				accessLevel integer,
				accessRole character varying,
				user character varying,
				password character varying,
				loginType character varying,
				PRIMARY KEY (role, path, method)
			);
			)kkt").arg(TBL_ACL_ACCESS));
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
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	for(const QString &s : ss.split(',', QString::SkipEmptyParts))
		ret.push_back(s.trimmed().toStdString());
#else
	for(const QString &s : ss.split(',', Qt::SkipEmptyParts))
		ret.push_back(s.trimmed().toStdString());
#endif
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
	for(std::string role : facl.accessRoles()) {
		AclRoleAccessRules rpt = facl.accessRoleRules(role);
		aclSetAccessRoleRules(role, rpt);
	}
}

std::vector<std::string> AclManagerSqlite::aclMountDeviceIds()
{
	return sqlLoadFields(TBL_ACL_MOUNTS, "deviceId");
}

AclMountDef AclManagerSqlite::aclMountDef(const std::string &device_id)
{
	AclMountDef ret;
	QSqlQuery q = sqlLoadRow(TBL_ACL_MOUNTS, "deviceId", QString::fromStdString(device_id));
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
		QString qs = "INSERT OR REPLACE INTO " + TBL_ACL_MOUNTS + " (deviceId, mountPoint, description) VALUES('%1', '%2', '%3')";
		qs = qs.arg(QString::fromStdString(device_id));
		qs = qs.arg(QString::fromStdString(md.mountPoint));
		qs = qs.arg(QString::fromStdString(md.description));
		logAclManagerM() << qs;
		execSql(qs);
	}
	else {
		QString qs = "DELETE FROM " + TBL_ACL_MOUNTS + " WHERE deviceId='" + QString::fromStdString(device_id) + "'";
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
		ret.weight = q.value("weight").toInt();
		ret.roles = split_str_vec(q.value("roles").toString());
		std::string profile_str = q.value("profile").toString().toStdString();
		if(!profile_str.empty()) {
			std::string err;
			ret.profile = shv::chainpack::RpcValue::fromCpon(profile_str, &err);
			if(!err.empty())
				shvError() << role_name << "invalid profile definition:" << profile_str;
		}
		if(!ret.profile.isMap())
			ret.profile = cp::RpcValue();
	}
	return ret;
}

void AclManagerSqlite::aclSetRole(const std::string &role_name, const AclRole &r)
{
	if(r.isValid()) {
		QString qs = "INSERT OR REPLACE INTO " + TBL_ACL_ROLES + " (name, weight, roles, profile) VALUES('%1', %2, '%3', '%4')";
		qs = qs.arg(QString::fromStdString(role_name));
		qs = qs.arg(r.weight);
		qs = qs.arg(join_str_vec(r.roles));
		qs = qs.arg(QString::fromStdString(r.profile.isValid()? r.profile.toCpon(): ""));
		logAclManagerM() << qs;
		execSql(qs);
	}
	else {
		execSql("DELETE FROM " + TBL_ACL_ROLES + " WHERE name='" + QString::fromStdString(role_name) + "'");
	}
}

std::vector<std::string> AclManagerSqlite::aclAccessRoles()
{
	std::vector<std::string> ret;
	QSqlQuery q = execSql("SELECT role FROM " + TBL_ACL_ACCESS + " GROUP BY role ORDER BY role");
	while (q.next()) {
		ret.push_back(q.value(0).toString().toStdString());
	}
	return ret;
}

AclRoleAccessRules AclManagerSqlite::aclAccessRoleRules(const std::string &role_name)
{
	AclRoleAccessRules ret;
	QSqlQuery q = execSql("SELECT * FROM " + TBL_ACL_ACCESS + " WHERE role='" + QString::fromStdString(role_name) + "'");
	while(q.next()) {
		AclAccessRule ag;
		ag.pathPattern = q.value("path").toString().toStdString();
		ag.method = q.value("method").toString().toStdString();
		//ag.forwardUserLoginFromRequest = q.value(PathAccessGrant::FORWARD_USER_LOGIN).toBool();
		std::string grant_type = q.value("grantType").toString().toStdString();
		ag.grant.type = cp::AccessGrant::typeFromString(grant_type);
		switch (ag.grant.type) {
		case cp::AccessGrant::Type::AccessLevel: ag.grant.accessLevel = q.value("accessLevel").toInt(); break;
		case cp::AccessGrant::Type::Role: ag.grant.role = q.value("accessRole").toString().toStdString(); break;
		case cp::AccessGrant::Type::UserLogin: {
			ag.grant.login.user = q.value("user").toString().toStdString();
			ag.grant.login.password = q.value("password").toString().toStdString();
			ag.grant.login.loginType = cp::UserLogin::loginTypeFromString(q.value("loginType").toString().toStdString());
			break;
		}
		default:
			SHV_EXCEPTION("Invalid PathAccessGrant type: " + grant_type);
		}
		ret.push_back(std::move(ag));
	}
	return ret;
}

void AclManagerSqlite::aclSetAccessRoleRules(const std::string &role_name, const AclRoleAccessRules &rules)
{
	QString qs = "DELETE FROM " + TBL_ACL_ACCESS + " WHERE role='" + QString::fromStdString(role_name) + "'";
	logAclManagerM() << qs;
	execSql(qs);
	if(!rules.empty()) {
		QSqlDatabase db = m_brokerApp->sqlConfigConnection();
		QSqlDriver *drv = db.driver();
		QSqlRecord rec = drv->record(TBL_ACL_ACCESS);
		for(const auto &rule : rules) {
			rec.setValue("role", QString::fromStdString(role_name));
			rec.setValue("path", QString::fromStdString(rule.pathPattern));
			rec.setValue("method", rule.method.empty()? QVariant(): QString::fromStdString(rule.method));
			rec.setValue("grantType", cp::AccessGrant::typeToString(rule.grant.type));
			switch (rule.grant.type) {
			case cp::AccessGrant::Type::AccessLevel: rec.setValue("accessLevel", rule.grant.accessLevel); break;
			case cp::AccessGrant::Type::Role: rec.setValue("accessRole", QString::fromStdString(rule.grant.role)); break;
			case cp::AccessGrant::Type::UserLogin: {
				rec.setValue("user", QString::fromStdString(rule.grant.login.user));
				rec.setValue("password", QString::fromStdString(rule.grant.login.password));
				rec.setValue("loginType", cp::UserLogin::loginTypeToString(rule.grant.login.loginType));
				break;
			}
			default:
				SHV_EXCEPTION("Invalid PathAccessGrant type: " + std::to_string((int)rule.grant.type));
			}
			qs = drv->sqlStatement(QSqlDriver::InsertStatement, TBL_ACL_ACCESS, rec, false);
			logAclManagerM() << qs;
			execSql(qs);
		}
	}
}

} // namespace broker
} // namespace shv
