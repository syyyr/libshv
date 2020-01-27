#include "aclmountdef.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace broker {

shv::chainpack::RpcValue AclMountDef::toRpcValueMap() const
{
	return shv::chainpack::RpcValue::Map {
		{"description", description},
		{"mountPoint", mountPoint},
	};
}

AclMountDef AclMountDef::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclMountDef ret;
	if(v.isMap()) {
		const auto &m = v.toMap();
		ret.description = m.value("description").toString();
		ret.mountPoint = m.value("mountPoint").toString();
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
