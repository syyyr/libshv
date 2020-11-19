#include "aclmountdef.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace broker {

shv::chainpack::RpcValue AclMountDef::toRpcValueMap() const
{
	shv::chainpack::RpcValue::Map m {
		{"mountPoint", mountPoint},
	};
	if(!description.empty())
		m["description"] = description;
	return m;
}

AclMountDef AclMountDef::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclMountDef ret;
	if(v.isString()) {
		ret.mountPoint = v.toString();
	}
	else if(v.isMap()) {
		const auto &m = v.toMap();
		ret.description = m.value("description").toString();
		ret.mountPoint = m.value("mountPoint").toString();
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
