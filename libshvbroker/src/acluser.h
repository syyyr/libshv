#pragma once

#include "shvbrokerglobal.h"
#include "aclpassword.h"

#include <vector>

namespace shv { namespace chainpack { class RpcValue; } }

namespace shv {
namespace broker {

struct SHVBROKER_DECL_EXPORT AclUser
{
	//std::string name;
	AclPassword password;
	std::vector<std::string> roles;

	AclUser() {}
	AclUser(AclPassword p, std::vector<std::string> roles)
		: password(std::move(p))
	    , roles(std::move(roles))
	{}
	//const std::string userName() const {return login.user;}
	bool isValid() const {return password.isValid();}
	shv::chainpack::RpcValue toRpcValueMap() const;
	static AclUser fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace chainpack
} // namespace shv

