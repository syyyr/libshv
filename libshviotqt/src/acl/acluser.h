#pragma once

#include "../shviotqtglobal.h"
#include "aclpassword.h"

#include <vector>

namespace shv { namespace chainpack { class RpcValue; } }

namespace shv {
namespace iotqt {
namespace acl {

struct SHVIOTQT_DECL_EXPORT AclUser
{
	AclPassword password;
	std::vector<std::string> roles;

	AclUser() = default;
	AclUser(AclPassword p, std::vector<std::string> roles_)
		: password(std::move(p))
		, roles(std::move(roles_))
	{}
	bool isValid() const {return password.isValid();}
	shv::chainpack::RpcValue toRpcValue() const;
	static AclUser fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace acl
} // namespace iotqt
} // namespace shv

