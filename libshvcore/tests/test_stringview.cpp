#include <shv/core/stringview.h>
#include <shv/core/utils.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace shv::core;
using namespace std;

DOCTEST_TEST_CASE("StringView")
{
	DOCTEST_SUBCASE("StringView::getToken()")
	{
		string str("///foo/bar//baz");
		StringView s(str);
		StringView s1 = utils::getToken(s, '/');
		REQUIRE(s1.length() == 0);
		s1 = utils::getToken(s.substr(1), '/');
		REQUIRE(s1.length() == 0);
		s1 = utils::getToken(s.substr(2), '/');
		REQUIRE(s1.length() == 0);
		s1 = utils::getToken(s.substr(3), '/');
		REQUIRE(s1 == "foo");
	}
	DOCTEST_SUBCASE("StringView::split()")
	{
		DOCTEST_SUBCASE("Single string")
		{
			string str("a:");
			StringView s(str);
			vector<StringView> string_list = utils::split(s, ':', '"', utils::SplitBehavior::KeepEmptyParts);
			REQUIRE(string_list.size() == 2);
			REQUIRE(string_list[0] == "a");
			REQUIRE(string_list[1].empty());
		}
		DOCTEST_SUBCASE("Quoted string")
		{
			string str("a:\"b:b\":c");
			StringView s(str);
			vector<StringView> string_list = utils::split(s, ':', '"', utils::SplitBehavior::KeepEmptyParts);
			REQUIRE(string_list.size() == 3);
			REQUIRE(string_list[0] == "a");
			REQUIRE(string_list[1] == "\"b:b\"");
			REQUIRE(string_list[2] == "c");
		}
		DOCTEST_SUBCASE("Consequent separators")
		{
			string str("///foo/bar//baz//");
			StringView s(str);
			vector<StringView> string_list1 = utils::split(s, '/');
			REQUIRE(string_list1.size() == 3);
			REQUIRE(string_list1[1] == "bar");
			vector<StringView> string_list2 = utils::split(s, '/', '"', utils::SplitBehavior::KeepEmptyParts);
			REQUIRE(string_list2.size() == 9);
			REQUIRE(string_list2[5].empty());
			REQUIRE(string_list2[6] == "baz");
		}
	}
}
