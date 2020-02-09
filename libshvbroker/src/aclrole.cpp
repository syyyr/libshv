#include "aclrole.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace broker {

shv::chainpack::RpcValue AclRole::toRpcValueMap() const
{
	return shv::chainpack::RpcValue::Map {
		//{"name", name},
		{"weight", weight},
		{"roles", shv::chainpack::RpcValue::List::fromStringList(roles)},
	};
}

AclRole AclRole::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclRole ret;
	if(v.isMap()) {
		const auto &m = v.toMap();
		//ret.name = m.value("name").toString();
		ret.weight = m.value("weight").toInt();
		std::vector<std::string> roles;
		for(auto v : m.value("roles").toList())
			roles.push_back(v.toString());
		// legacy key for 'roles' was 'grants'
		for(auto v : m.value("grants").toList())
			roles.push_back(v.toString());
		ret.roles = roles;
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
