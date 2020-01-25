#include "aclmountdef.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

RpcValue AclMountDef::toRpcValueMap() const
{
	return RpcValue::Map {
		{"description", description},
		{"mountPoint", mountPoint},
	};
}

AclMountDef AclMountDef::fromRpcValue(const RpcValue &v)
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
