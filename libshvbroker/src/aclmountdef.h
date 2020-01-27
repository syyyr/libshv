#pragma once

#include "shvbrokerglobal.h"

#include <string>

namespace shv { namespace chainpack { class RpcValue; } }

namespace shv {
namespace broker {

struct SHVBROKER_DECL_EXPORT AclMountDef
{
	//std::string deviceId;
	std::string mountPoint;
	std::string description;

	bool isValid() const {return !mountPoint.empty();}
	shv::chainpack::RpcValue toRpcValueMap() const;
	static AclMountDef fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace chainpack
} // namespace shv

