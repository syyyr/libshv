#pragma once

#include "../shvchainpackglobal.h"
#include "accessgrant.h"

namespace shv {
namespace chainpack {

struct SHVCHAINPACK_DECL_EXPORT AclRolePaths : public std::map<std::string, PathAccessGrant>
{
	bool isValid() const {return !empty();}
	RpcValue toRpcValueMap() const;
	static AclRolePaths fromRpcValue(const RpcValue &v);
};

} // namespace chainpack
} // namespace shv

