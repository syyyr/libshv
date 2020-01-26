#ifndef SHV_CHAINPACK_ACLMOUNTDEF_H
#define SHV_CHAINPACK_ACLMOUNTDEF_H

#include "../shvchainpackglobal.h"

#include <string>

namespace shv {
namespace chainpack {

class RpcValue;

struct SHVCHAINPACK_DECL_EXPORT AclMountDef
{
	//std::string deviceId;
	std::string mountPoint;
	std::string description;

	bool isValid() const {return !mountPoint.empty();}
	RpcValue toRpcValueMap() const;
	static AclMountDef fromRpcValue(const RpcValue &v);
};

} // namespace chainpack
} // namespace shv

#endif // SHV_CHAINPACK_ACLMOUNTDEF_H
