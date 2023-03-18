#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/chainpack/accessgrant.h>

#include <necrolog.h>

#include <doctest/doctest.h>

#include <optional>

using namespace shv::chainpack;
using namespace std;
using namespace std::string_literals;

string list_to_string(const vector<string_view>& value) {
	RpcValue::List lst;
	for(const auto &s : value)
		lst.push_back(string(s));
	return RpcValue(lst).toCpon();
}

bool cmp_stringview_list(const vector<string_view>& lst1, const vector<string_view>& lst2) {
	if(lst1.size() != lst2.size())
		return false;
	for(size_t i = 0; i< lst1.size(); ++i)
		if(lst1[i] != lst2[i])
			return false;
	return true;
}

namespace shv::chainpack {

doctest::String toString(const RpcValue& value) {
	return value.toCpon().c_str();
}

doctest::String toString(const RpcValue::DateTime& value) {
	return value.toIsoString().c_str();
}

doctest::String toString(const vector<string_view>& value) {
	return list_to_string(value).c_str();
}

}

DOCTEST_TEST_CASE("AccessGrant")
{
	AccessGrant acg;
	vector<string_view> expected_roles;
	DOCTEST_SUBCASE("Empty") {
		acg = AccessGrant("");
		expected_roles = {};
	}
	DOCTEST_SUBCASE("Single") {
		acg = AccessGrant("wr");
		expected_roles = {"wr"};
	}
	DOCTEST_SUBCASE("Multiple") {
		acg = AccessGrant("rd,dot_local");
		expected_roles = {"rd", "dot_local"};
	}
	auto roles = acg.roles();
	//nInfo() << "roles         :" << list_to_string(roles);
	//nInfo() << "expected_roles:" << list_to_string(expected_roles);
	REQUIRE(cmp_stringview_list(expected_roles, roles));
}
