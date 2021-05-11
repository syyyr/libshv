#pragma once

#include "../shviotqtglobal.h"

#include "shv/chainpack/rpcvalue.h"

#include <string>
#include <vector>

namespace shv {
namespace iotqt {
namespace acl {

struct SHVIOTQT_DECL_EXPORT AclRole
{
	static constexpr int INVALID_WEIGHT = std::numeric_limits<int>::min();
	//std::string name;
	int weight = INVALID_WEIGHT;
	std::vector<std::string> roles;
	shv::chainpack::RpcValue profile;

	AclRole() {}
	//AclRole(std::string n, int w = 0) : name(std::move(n)), weight(w) {}
	//AclRole(std::string n, int w, std::vector<std::string> roles) : name(std::move(n)), weight(w), roles(std::move(roles)) {}
	AclRole(int w) : weight(w) {}
	AclRole(int w, std::vector<std::string> roles) : weight(w), roles(std::move(roles)) {}

	bool isValid() const {return weight != INVALID_WEIGHT;}
	shv::chainpack::RpcValue toRpcValue() const;
	static AclRole fromRpcValue(const shv::chainpack::RpcValue &v);
};

} // namespace acl
} // namespace iotqt
} // namespace shv

