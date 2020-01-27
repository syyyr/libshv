#include "aclrolepaths.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace broker {

shv::chainpack::RpcValue AclRolePaths::toRpcValueMap() const
{
	shv::chainpack::RpcValue::Map ret;
	for(auto kv : *this) {
		ret[kv.first] = kv.second.toRpcValue();
	}
	return shv::chainpack::RpcValue(std::move(ret));
}

AclRolePaths AclRolePaths::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclRolePaths ret;
	if(v.isMap()) {
		const auto &m = v.toMap();
		for(auto kv : m) {
			ret[kv.first] = shv::chainpack::PathAccessGrant::fromRpcValue(kv.second);
		}
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
