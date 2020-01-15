#ifndef SHV_CHAINPACK_ACLUSERDEF_H
#define SHV_CHAINPACK_ACLUSERDEF_H

#include "../shvchainpackglobal.h"
#include "accessgrant.h"

namespace shv {
namespace chainpack {

struct SHVCHAINPACK_DECL_EXPORT AclUserDef
{
	AccessGrant user;
	std::vector<std::string> roles;
};

} // namespace chainpack
} // namespace shv

#endif // SHV_CHAINPACK_ACLUSERDEF_H
