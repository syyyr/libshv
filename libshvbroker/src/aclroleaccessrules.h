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

	chainpack::RpcValue toRpcValue() const;
	static AclAccessRule fromRpcValue(const chainpack::RpcValue &rpcval);

	bool isValid() const { return !pathPattern.empty() && grant.isValid(); }
	bool isMoreSpecificThan(const AclAccessRule &other) const;
	bool isPathMethodMatch(const std::string &shv_path, const std::string &method) const;
};

class SHVBROKER_DECL_EXPORT AclRoleAccessRules : public std::vector<AclAccessRule>
{
public:
	shv::chainpack::RpcValue toRpcValue() const;
	shv::chainpack::RpcValue toRpcValue_legacy() const;

	//void sortMostSpecificFirst();

	static AclRoleAccessRules fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace chainpack
} // namespace shv

