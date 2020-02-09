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
			auto g = shv::chainpack::PathAccessGrant::fromRpcValue(kv.second);
			if(g.isValid())
				ret[kv.first] = std::move(g);
		}
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
