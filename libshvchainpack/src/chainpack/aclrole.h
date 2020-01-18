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
	AclRole(std::string n, int w = 0) : name(std::move(n)), weight(w) {}
	AclRole(std::string n, int w, std::vector<std::string> roles) : name(std::move(n)), weight(w), roles(std::move(roles)) {}

	bool isValid() const {return !name.empty();}
};

} // namespace chainpack
} // namespace shv

