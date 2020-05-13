#pragma once

#include "shvbrokerglobal.h"

#include <shv/chainpack/accessgrant.h>
#include <shv/core/utils/shvpath.h>

#include <map>

namespace shv {
namespace broker {

struct SHVBROKER_DECL_EXPORT AclAccessRule
{
public:
	std::string pathPattern;
	std::string method;
	shv::chainpack::AccessGrant grant;

	AclAccessRule() {}
	AclAccessRule(const std::string &path_pattern, const std::string &method = std::string())
		: pathPattern(path_pattern), method(method) {}
	AclAccessRule(const std::string &path_pattern, const std::string &method, const shv::chainpack::AccessGrant &grant)
		: pathPattern(path_pattern), method(method), grant(grant) {}
	//PathAccessGrant(Super &&o) : Super(std::move(o)) {}

	chainpack::RpcValue toRpcValue() const;
	static AclAccessRule fromRpcValue(const chainpack::RpcValue &rpcval);

	bool isValid() const { return !pathPattern.empty() && grant.isValid(); }
};

struct SHVBROKER_DECL_EXPORT AclRoleAccessRules : public std::vector<AclAccessRule>
{
	bool isValid() const {return !empty();}
	shv::chainpack::RpcValue toRpcValue() const;
	shv::chainpack::RpcValue toRpcValue_legacy() const;
	static AclRoleAccessRules fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace chainpack
} // namespace shv

