#include "aclmanager.h"

#include <shv/broker/brokerapp.h>

namespace cp = shv::chainpack;

AclManager::AclManager(shv::broker::BrokerApp *broker_app)
	: Super(broker_app)
{
}

namespace {
std::map<std::string, shv::broker::AclUser> s_aclUsers = {
	{"guest", {{"password", shv::broker::AclPassword::Format::Plain}, {"user"}}},
	{"poweruser", {{"weakpassword", shv::broker::AclPassword::Format::Plain}, {"user", "system"}}},
	{"admin", {{"19b9eab2dea2882d328caa6bc26b0b66c002813b", shv::broker::AclPassword::Format::Sha1}, {"superuser"}}},
};
}

std::vector<std::string> AclManager::aclUsers()
{
	return cp::Utils::mapKeys(s_aclUsers);
}

shv::broker::AclUser AclManager::aclUser(const std::string &user_name)
{
	auto it = s_aclUsers.find(user_name);
	return (it == s_aclUsers.end())? shv::broker::AclUser(): it->second;
}

namespace {
std::map<std::string, shv::broker::AclRole> s_aclRoles = {
	{"user", {0}},
	{"system", {5}},
	{"superuser", {10, {"user", "system"}}},
};
}

std::vector<std::string> AclManager::aclRoles()
{
	return cp::Utils::mapKeys(s_aclRoles);
}

shv::broker::AclRole AclManager::aclRole(const std::string &role_name)
{
	auto it = s_aclRoles.find(role_name);
	return (it == s_aclRoles.end())? shv::broker::AclRole(): it->second;
}


std::vector<std::string> AclManager::aclAccessRoles()
{
	return cp::Utils::mapKeys(s_aclRoles);
}

shv::broker::AclRolePaths AclManager::aclAccessRolePaths(const std::string &role_name)
{
	shv::broker::AclRolePaths ret;
	if(role_name == "user") {
		{
			cp::AccessGrant &grant = ret["**"];
			grant.type = cp::AccessGrant::Type::AccessLevel;
			//grant.role = cp::Rpc::ROLE_BROWSE;
			grant.accessLevel = cp::MetaMethod::AccessLevel::Browse;
		}
		{
			cp::AccessGrant &grant = ret["test/**"];
			grant.type = cp::AccessGrant::Type::Role;
			grant.role = cp::Rpc::ROLE_READ;
		}
	}
	if(role_name == "system") {
		{
			cp::AccessGrant &grant = ret[".broker/**"];
			grant.type = cp::AccessGrant::Type::AccessLevel;
			//grant.role = cp::Rpc::ROLE_BROWSE;
			grant.accessLevel = cp::MetaMethod::AccessLevel::Service;
		}
		{
			cp::AccessGrant &grant = ret["test/**"];
			grant.type = cp::AccessGrant::Type::Role;
			grant.role = cp::Rpc::ROLE_SERVICE;
		}
	}
	if(role_name == "superuser") {
		{
			cp::AccessGrant &grant = ret["**"];
			grant.type = cp::AccessGrant::Type::AccessLevel;
			//grant.role = cp::Rpc::ROLE_BROWSE;
			grant.accessLevel = cp::MetaMethod::AccessLevel::Admin;
		}
	}
	return ret;
}
