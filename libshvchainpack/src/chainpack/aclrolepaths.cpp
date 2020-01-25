#include "aclrolepaths.h"

namespace shv {
namespace chainpack {

RpcValue AclRolePaths::toRpcValueMap() const
{
	RpcValue::Map ret;
	for(auto kv : *this) {
		ret[kv.first] = kv.second.toRpcValue();
	}
	return RpcValue(std::move(ret));
}

AclRolePaths AclRolePaths::fromRpcValue(const RpcValue &v)
{
	AclRolePaths ret;
	if(v.isMap()) {
		const auto &m = v.toMap();
		for(auto kv : m) {
			ret[kv.first] = PathAccessGrant::fromRpcValue(kv.second);
		}
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
