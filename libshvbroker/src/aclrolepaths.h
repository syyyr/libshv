#pragma once

#include "shvbrokerglobal.h"

#include <shv/chainpack/accessgrant.h>

#include <map>

namespace shv {
namespace broker {

struct SHVBROKER_DECL_EXPORT AclRolePaths : public std::map<std::string, shv::chainpack::PathAccessGrant>
{
	bool isValid() const {return !empty();}
	shv::chainpack::RpcValue toRpcValueMap() const;
	static AclRolePaths fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace chainpack
} // namespace shv

