#include "aclrole.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

RpcValue AclRole::toRpcValueMap() const
{
	return RpcValue::Map {
		{"name", name},
		{"weight", weight},
		{"roles", RpcValue::List::fromStringList(roles)},
	};
}

AclRole AclRole::fromRpcValue(const RpcValue &v)
{
	AclRole ret;
	if(v.isMap()) {
		const auto &m = v.toMap();
		ret.name = m.value("name").toString();
		ret.weight = m.value("weight").toInt();
		std::vector<std::string> roles;
		for(auto v : m.value("roles").toList())
			roles.push_back(v.toString());
		ret.roles = roles;
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
