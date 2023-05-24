#pragma once
#include <string>
#include <vector>

struct LdapConfig {
	std::string hostName;
	std::string searchBaseDN;
	std::vector<std::string> searchAttrs;
	struct GroupMapping {
		std::string ldapGroup;
		std::string shvGroup;
	};
	std::vector<GroupMapping> groupMapping;
};
