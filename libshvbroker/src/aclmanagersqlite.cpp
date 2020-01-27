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

namespace cp = shv::chainpack;

namespace shv {
namespace broker {

const auto DB_CONN_NAME = QStringLiteral("AclManagerSqlite");

const auto TBL_FSTAB = QStringLiteral("fstab");
const auto TBL_USERS = QStringLiteral("users");
const auto TBL_ROLES = QStringLiteral("roles");
const auto TBL_PATHS = QStringLiteral("paths");

AclManagerSqlite::AclManagerSqlite(BrokerApp *broker_app)
	: Super(broker_app)
{
}

AclManagerSqlite::~AclManagerSqlite()
{
}

QSqlQuery AclManagerSqlite::execSql(const QString &query_str)
{
	QSqlDatabase db = QSqlDatabase::database(DB_CONN_NAME, true);
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

void AclManagerSqlite::initDbConnection()
{
	AppCliOptions *opts = m_brokerApp->cliOptions();
	if(opts->aclSqlDriver() == "QSQLITE") {
		std::string fn = opts->aclSqlDatabase();
		if(fn.empty())
			SHV_EXCEPTION("SQL ACL Manager database not set.");
		if(fn[0] != '/')
			fn = opts->configDir() + '/' + fn;
		shvInfo() << "Opening SQLite ACL database:" << fn;
		QString qfn = QString::fromStdString(fn);
		if(!QFile::exists(qfn)) {
			shvInfo() << "Creating SQLite ACL database:" << fn;
			createSqliteDatabase(qfn);
		}
		try {
			QSqlQuery q = execSql("SELECT COUNT(*) FROM users");
			if(q.next()) {
				if(q.value(0).toInt() > 0)
					return;
				importFileAclConfig();
			}
			SHV_EXCEPTION("SELECT COUNT(*) returned no record, this should never happen");
		}
		catch (const shv::core::Exception &e) {
			//shvError() << "SQLite ACL DB currupted!";
			SHV_EXCEPTION("SQLite ACL DB currupted: " + e.message());
		}
	}
	else {
		SHV_EXCEPTION("ACL Manager for SQL driver " + opts->aclSqlDriver() + " is not supported.");
	}
}

void AclManagerSqlite::createSqliteDatabase(const QString &file_name)
{
	QSqlDatabase db = QSqlDatabase::database(DB_CONN_NAME, false);
	db.setDatabaseName(file_name);
	if(!db.open())
		SHV_EXCEPTION("Cannot open SQLite ACL database " + file_name.toStdString());
	execSql("BEGIN TRANSACTION");
	execSql(QStringLiteral(R"kkt(
			CREATE TABLEIF NOT EXISTS %1 (
			deviceId character varying PRIMARY KEY,
			mountPoint character varying,
			description character varying
			);
			)kkt").arg(TBL_FSTAB));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
			name character varying PRIMARY KEY,
			password character varying,
			passwordFormat character varying,
			TBL_ROLES character varying
			);
			)kkt").arg(TBL_USERS));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
			name character varying PRIMARY KEY,
			weight integer,
			roles character varying
			);
			)kkt").arg(TBL_ROLES));
	execSql(QStringLiteral(R"kkt(
			CREATE TABLE IF NOT EXISTS %1 (
			role character varying,
			path character varying,
			forwardGrant boolean,
			grantType character varying,
			accessLevel integer,
			accessRole character varying,
			user character varying,
			password character varying,
			loginType character varying,
			CONSTRAINT %1_unique0 UNIQUE (role, path)
			);
			)kkt").arg(TBL_PATHS));
	execSql("COMMIT");
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

void AclManagerSqlite::importFileAclConfig()
{
	execSql("BEGIN TRANSACTION");
	AclManagerConfigFiles facl(m_brokerApp);
	for(std::string id : facl.mountDeviceIds()) {
		AclMountDef md = facl.mountDef(id);
		aclSetMountDef(id, md);
	}
	for(std::string user : facl.users()) {
		AclUser u = facl.user(user);
		aclSetUser(user, u);
	}
	for(std::string role : facl.roles()) {
		AclRole r = facl.role(role);
		aclSetRole(role, r);
		/*
		QString qs = "INSERT INTO users (name, weight, roles) VALUES('%1', '%2', '%3')";
		qs = qs.arg(QString::fromStdString(role));
		qs = qs.arg(r.weight);
		qs = qs.arg(join_str_vec(r.roles));
		execSql(qs);
		*/
	}
	for(std::string role : facl.pathsRoles()) {
		AclRolePaths rpt = facl.pathsRolePaths(role);
		aclSetRolePaths(role, rpt);
	}
#if 0
	{
		QString qs = "INSERT INTO " + TBL_PATHS + " (role, path, forwardGrant, grantType"
					 ", accessLevel"
					 ", accessRole"
					 ", user, password, loginType)"
					 " VALUES (:role, :path, :forwardGrant, :grantType"
					 ", :accessLevel"
					 ", :accessRole"
					 ", :user, :password, :loginType)";
		QSqlDatabase db = QSqlDatabase::database(DB_CONN_NAME, true);
		QSqlQuery q(db);
		if(!q.prepare(qs))
			SHV_EXCEPTION("SQL ERROR: " + q.lastError().text().toStdString() + "\nQuery: " + qs.toStdString());
		for(std::string role : facl.pathsRoles()) {
			AclRolePaths rpt = facl.pathsRolePaths(role);
			for(const auto &kv : rpt) {
				enum {ColRole = 0, ColPath
					  , ColForwardGrant
					  , ColGrantType
					  , ColAccessLevel
					  , ColAccessRole
					  , ColUser, ColPassword, ColPasswordFormat
					  , ColCNT};
				QVariantList values;
				for (int i = 0; i < ColCNT; ++i)
					values[i] = QVariant();
				const cp::PathAccessGrant &g = kv.second;
				values[ColRole] = QString::fromStdString(role);
				values[ColPath] = QString::fromStdString(kv.first);
				if(g.forwardGrantFromRequest)
					values[ColForwardGrant] = true;
				values[ColGrantType] = cp::PathAccessGrant::typeToString(g.type);
				switch (g.type) {
				case cp::PathAccessGrant::Type::AccessLevel: values[ColAccessLevel] = g.accessLevel; break;
				case cp::PathAccessGrant::Type::Role: values[ColAccessRole] = QString::fromStdString(g.role); break;
				case cp::PathAccessGrant::Type::UserLogin: {
					values[ColUser] = QString::fromStdString(g.login.user);
					values[ColPassword] = QString::fromStdString(g.login.password);
					values[ColPasswordFormat] = cp::UserLogin::loginTypeToString(g.login.loginType);
					break;
				}
				default:
					SHV_EXCEPTION("Invalid PathAccessGrant type: " + std::to_string((int)g.type));
				}
				q.bindValue(QStringLiteral(":role"), values[ColRole]);
				q.bindValue(QStringLiteral(":path"), values[ColPath]);
				q.bindValue(QStringLiteral(":forwardGrant"), values[ColForwardGrant]);
				q.bindValue(QStringLiteral(":grantType"), values[ColGrantType]);
				q.bindValue(QStringLiteral(":accessLevel"), values[ColAccessLevel]);
				q.bindValue(QStringLiteral(":accessRole"), values[ColAccessRole]);
				q.bindValue(QStringLiteral(":user"), values[ColUser]);
				q.bindValue(QStringLiteral(":password"), values[ColPassword]);
				q.bindValue(QStringLiteral(":loginType"), values[ColPasswordFormat]);
				if(!q.exec())
					SHV_EXCEPTION("SQL ERROR: " + q.lastError().text().toStdString());
			}
		}
	}
#endif
	execSql("COMMIT");
}

std::vector<std::string> AclManagerSqlite::aclMountDeviceIds()
{
	return sqlLoadFields(TBL_FSTAB, "deviceId");
}

AclMountDef AclManagerSqlite::aclMountDef(const std::string &device_id)
{
	AclMountDef ret;
	QSqlQuery q = sqlLoadRow(TBL_FSTAB, "deviceId", QString::fromStdString(device_id));
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
		QString qs = "INSERT INTO " + TBL_FSTAB + " (deviceId, mountPoint, description) VALUES('%1', '%2', '%3')";
		qs = qs.arg(QString::fromStdString(device_id));
		qs = qs.arg(QString::fromStdString(md.mountPoint));
		qs = qs.arg(QString::fromStdString(md.description));
		execSql(qs);
	}
	else {
		execSql("DELETE FROM " + TBL_FSTAB + " WHERE deviceId='" + QString::fromStdString(device_id) + "'");
	}
}

std::vector<std::string> AclManagerSqlite::aclUsers()
{
	return sqlLoadFields(TBL_USERS, "name");
}

AclUser AclManagerSqlite::aclUser(const std::string &user_name)
{
	AclUser ret;
	QSqlQuery q = sqlLoadRow(TBL_USERS, "name", QString::fromStdString(user_name));
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
		QString qs = "INSERT OR REPLACE INTO " + TBL_USERS + " (name, password, passwordFormat, roles) VALUES('%1', '%2', '%3', '%4')";
		qs = qs.arg(QString::fromStdString(user_name));
		qs = qs.arg(QString::fromStdString(u.password.password));
		qs = qs.arg(QString::fromStdString(AclPassword::formatToString(u.password.format)));
		qs = qs.arg(join_str_vec(u.roles));
		execSql(qs);
	}
	else {
		execSql("DELETE FROM " + TBL_USERS + " WHERE name='" + QString::fromStdString(user_name) + "'");
	}
}

std::vector<std::string> AclManagerSqlite::aclRoles()
{
	return sqlLoadFields(TBL_ROLES, "name");
}

AclRole AclManagerSqlite::aclRole(const std::string &role_name)
{
	AclRole ret;
	QSqlQuery q = sqlLoadRow(TBL_ROLES, "name", QString::fromStdString(role_name));
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
		QString qs = "INSERT OR REPLACE INTO " + TBL_ROLES + " (name, weight, roles) VALUES('%1', %2, '%3')";
		qs = qs.arg(QString::fromStdString(role_name));
		qs = qs.arg(r.weight);
		qs = qs.arg(join_str_vec(r.roles));
		execSql(qs);
	}
	else {
		execSql("DELETE FROM " + TBL_ROLES + " WHERE name='" + QString::fromStdString(role_name) + "'");
	}
}

std::vector<std::string> AclManagerSqlite::aclPathsRoles()
{
	std::vector<std::string> ret;
	QSqlQuery q = execSql("SELECT role FROM " + TBL_PATHS + " GROUP BY role ORDER BY role");
	while (q.next()) {
		ret.push_back(q.value(0).toString().toStdString());
	}
	return ret;
}

AclRolePaths AclManagerSqlite::aclPathsRolePaths(const std::string &role_name)
{
	AclRolePaths ret;
	QSqlQuery q = execSql("SELECT * FROM " + TBL_PATHS + " WHERE role='" + QString::fromStdString(role_name) + "'");
	while(q.next()) {
		QString path = q.value("path").toString();
		cp::PathAccessGrant ag;
		ag.forwardGrantFromRequest = q.value("forwardGrant").toBool();
		ag.type = static_cast<cp::PathAccessGrant::Type>(q.value("grantType").toInt());
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
			SHV_EXCEPTION("Invalid PathAccessGrant type: " + std::to_string((int)ag.type));
		}
	}
	return ret;
}

void AclManagerSqlite::aclSetRolePaths(const std::string &role_name, const AclRolePaths &rpt)
{
	execSql("DELETE FROM " + TBL_PATHS + " WHERE role='" + QString::fromStdString(role_name) + "'");
	if(rpt.isValid()) {
		QSqlDatabase db = QSqlDatabase::database(DB_CONN_NAME, true);
		QSqlDriver *drv = db.driver();
		QSqlRecord rec = drv->record(TBL_PATHS);
		for(const auto &kv : rpt) {
			rec.setValue("role", QString::fromStdString(role_name));
			rec.setValue("path", QString::fromStdString(kv.first));
			const cp::PathAccessGrant &g = kv.second;
			if(g.forwardGrantFromRequest)
				rec.setValue("forwardGrant", g.forwardGrantFromRequest);
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
			QString qs = drv->sqlStatement(QSqlDriver::InsertStatement, TBL_PATHS, rec, false);
			execSql(qs);
		}
	}
}

} // namespace broker
} // namespace shv
