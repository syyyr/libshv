#include "acluser.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv::iotqt::acl {

shv::chainpack::RpcValue AclUser::toRpcValue() const
{
	return shv::chainpack::RpcValue::Map {
		//{"name", name},
		{"password", password.toRpcValue()},
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
		for(const auto &lst : m.value("roles").toList())
			roles.push_back(lst.toString());
		// legacy key for 'roles' was 'grants'
		for(const auto &lst : m.value("grants").toList())
			roles.push_back(lst.toString());
		ret.roles = roles;
	}
	return ret;
}

} // namespace shv
