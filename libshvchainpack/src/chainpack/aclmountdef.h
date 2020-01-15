#ifndef SHV_CHAINPACK_ACLMOUNTDEF_H
#define SHV_CHAINPACK_ACLMOUNTDEF_H

#include "../shvchainpackglobal.h"

#include <string>

namespace shv {
namespace chainpack {

struct SHVCHAINPACK_DECL_EXPORT AclMountDef
{
	std::string description;
	std::string mountPoint;
};

} // namespace chainpack
} // namespace shv

#endif // SHV_CHAINPACK_ACLMOUNTDEF_H
