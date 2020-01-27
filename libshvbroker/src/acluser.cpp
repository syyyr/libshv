#include "acluser.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace broker {

shv::chainpack::RpcValue AclUser::toRpcValueMap() const
{
	return shv::chainpack::RpcValue::Map {
		//{"name", name},
		{"password", password.toRpcValueMap()},
		{"roles", shv::chainpack::RpcValue::List::fromStringList(roles)},
	};
}

AclUser AclUser::fromRpcValue(const shv::chainpack::RpcValue &v)
{
	AclUser ret;
	if(v.isMap()) {
		const auto &m = v.toMap();
		//ret.name = m.value("name").toString();
		ret.password = AclPassword::fromRpcValue(m.value("password"));
		std::vector<std::string> roles;
		for(auto v : m.value("roles").toList())
			roles.push_back(v.toString());
		ret.roles = roles;
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
