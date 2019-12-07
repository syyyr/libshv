#pragma once

#include "../shvchainpackglobal.h"
#include "accessgrant.h"

namespace shv {
namespace chainpack {

struct SHVCHAINPACK_DECL_EXPORT AclRolePaths : public std::map<std::string, AccessGrant>
{
};

} // namespace chainpack
} // namespace shv

