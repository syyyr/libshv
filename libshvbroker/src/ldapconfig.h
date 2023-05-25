#pragma once
#include <optional>
#include <string>
#include <vector>

struct LdapConfig {
	std::optional<std::string> brokerUsername;
	std::optional<std::string> brokerPassword;
	std::string hostName;
	std::string searchBaseDN;
	std::vector<std::string> searchAttrs;
	struct GroupMapping {
		std::string ldapGroup;
		std::string shvGroup;
	};
	std::vector<GroupMapping> groupMapping;
};
