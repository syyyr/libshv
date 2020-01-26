#ifndef SHV_CHAINPACK_ACLUSER_H
#define SHV_CHAINPACK_ACLUSER_H

#include "../shvchainpackglobal.h"
#include "aclpassword.h"

#include <vector>

namespace shv {
namespace chainpack {

class RpcValue;

struct SHVCHAINPACK_DECL_EXPORT AclUser
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
	RpcValue toRpcValueMap() const;
	static AclUser fromRpcValue(const RpcValue &v);
};

} // namespace chainpack
} // namespace shv

#endif // SHV_CHAINPACK_ACLUSERDEF_H
