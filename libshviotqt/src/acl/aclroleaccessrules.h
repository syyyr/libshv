#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/accessgrant.h>
#include <shv/core/utils/shvpath.h>

#include <map>

namespace shv {
namespace core { namespace utils { class ShvUrl; } }
namespace iotqt {
namespace acl {

struct SHVIOTQT_DECL_EXPORT AclAccessRule
{
public:
	std::string service;
	std::string pathPattern;
	std::string method;
	shv::chainpack::AccessGrant grant;

	static constexpr auto ALL_SERVICES = "*";

	AclAccessRule() = default;
	AclAccessRule(const std::string &path_pattern_, const std::string &method_ = std::string())
		: pathPattern(path_pattern_), method(method_) {}
	AclAccessRule(const std::string &path_pattern_, const std::string &method_, const shv::chainpack::AccessGrant &grant_)
		: pathPattern(path_pattern_), method(method_), grant(grant_) {}

	chainpack::RpcValue toRpcValue() const;
	static AclAccessRule fromRpcValue(const chainpack::RpcValue &rpcval);

	bool isValid() const { return !pathPattern.empty() && grant.isValid(); }
	bool isMoreSpecificThan(const AclAccessRule &other) const;
	bool isPathMethodMatch(const shv::core::utils::ShvUrl &shv_url, const std::string &method) const;
};

class SHVIOTQT_DECL_EXPORT AclRoleAccessRules : public std::vector<AclAccessRule>
{
public:
	shv::chainpack::RpcValue toRpcValue() const;
	shv::chainpack::RpcValue toRpcValue_legacy() const;

	static AclRoleAccessRules fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace acl
} // namespace iotqt
} // namespace shv

