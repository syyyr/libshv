#include "acluser.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

RpcValue AclUser::toRpcValueMap() const
{
	return RpcValue::Map {
		//{"name", name},
		{"password", password.toRpcValueMap()},
		{"roles", RpcValue::List::fromStringList(roles)},
	};
}

AclUser AclUser::fromRpcValue(const RpcValue &v)
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
