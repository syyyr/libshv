#pragma once

#include "../shvchainpackglobal.h"

#include <string>
#include <vector>

namespace shv {
namespace chainpack {

struct SHVCHAINPACK_DECL_EXPORT AclRole
{
	std::string name;
	int weight = 0;
	std::vector<std::string> roles;

	AclRole() {}
	AclRole(const std::string &n, int w) : name(n), weight(w) {}

	bool isValid() const {return !name.empty();}
};

} // namespace chainpack
} // namespace shv

