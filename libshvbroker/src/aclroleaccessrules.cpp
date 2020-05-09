#include "aclroleaccessrules.h"

#include <shv/chainpack/rpcvalue.h>

#include <QString>

using namespace shv::chainpack;
//namespace cp = shv::chainpack;

namespace shv {
namespace broker {

//================================================================
// PathAccessGrant
//================================================================
//const char* PathAccessGrant::FORWARD_USER_LOGIN = "forwardLogin";

RpcValue AclAccessRule::toRpcValue() const
{
	RpcValue::Map m = grant.toRpcValueMap().toMap();
	m["method"] = method;
	m["pathPattern"] = pathPattern;
	return std::move(m);
}

AclAccessRule AclAccessRule::fromRpcValue(const RpcValue &rpcval)
{
	AclAccessRule ret;
	ret.grant = AccessGrant::fromRpcValue(rpcval);
	ret.method = rpcval.at("method").toString();
	ret.pathPattern = rpcval.at("pathPattern").toString();
	return ret;
}

//================================================================
// AclRolePaths
//================================================================
shv::chainpack::RpcValue AclRoleAccessRules::toRpcValue() const
{
	shv::chainpack::RpcValue::List ret;
	for(auto kv : *this) {
		ret.push_back(kv.toRpcValue());
	}
	return shv::chainpack::RpcValue(std::move(ret));
}

AclRoleAccessRules AclRoleAccessRules::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclRoleAccessRules ret;
	if(v.isMap()) {
		const auto &m = v.toMap();
		for(auto kv : m) {
			auto g = AclAccessRule::fromRpcValue(kv.second);
			auto i = kv.first.find_last_of(shv::core::utils::ShvPath::SHV_PATH_METHOD_DELIM);
			if(i == std::string::npos) {
				g.pathPattern = kv.first;
			}
			else {
				g.pathPattern = kv.first.substr(0, i);
				g.method = kv.first.substr(i + 1);
			}
			if(g.isValid())
				ret.push_back(std::move(g));
		}
	}
	else if(v.isList()) {
		const auto &l = v.toList();
		for(auto kv : l) {
			auto g = AclAccessRule::fromRpcValue(kv);
			if(g.isValid())
				ret.push_back(std::move(g));
		}
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
