#include <shv/core/stringview.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace shv::core;
using namespace std;

DOCTEST_TEST_CASE("StringView")
{
	DOCTEST_SUBCASE("Normalization")
	{
		DOCTEST_SUBCASE("Empty string")
		{
			string str;
			StringView s1(str);
			StringView s2(str, 5);
			StringView s3(str, 6, 1000);
			StringView s4(str, 7, 0);
			StringView s5(str, 0, 0);
			REQUIRE(s1.valid());
			REQUIRE(s1.empty());
			REQUIRE(s2.valid());
			REQUIRE(s2.empty());
			REQUIRE(s3.valid());
			REQUIRE(s3.empty());
			REQUIRE(s4.valid());
			REQUIRE(s4.empty());
			REQUIRE(s5.valid());
			REQUIRE(s5.empty());
		}
		DOCTEST_SUBCASE("Not empty string")
		{
			string str("abcde");
			StringView s1(str);
			StringView s2(str, 5);
			StringView s3(str, 2, 1000);
			StringView s4(str, 7, 0);
			StringView s5(str, 0, 0);
			StringView s6(str, 1, 1);
			REQUIRE(s1.valid());
			REQUIRE(!s1.empty());
			REQUIRE(s1.length() == 5);
			REQUIRE(s2.valid());
			REQUIRE(s2.empty());
			REQUIRE(s3.valid());
			REQUIRE(!s3.empty());
			REQUIRE(s3.length() == 3);
			REQUIRE(s3.toString() == "cde");
			REQUIRE(s4.valid());
			REQUIRE(s4.empty());
			REQUIRE(s5.valid());
			REQUIRE(s5.empty());
			REQUIRE(s6.valid());
			REQUIRE(!s6.empty());
			REQUIRE(s6.length() == 1);
			REQUIRE(s6.toString() == "b");
		}
	}
	DOCTEST_SUBCASE("StringView::mid()")
	{
		string str("1234567890");
		StringView s(str);
		StringView s1 = s.mid(0);
		REQUIRE(s1.length() == 10);
		StringView s11 = s.mid(1, 2);
		REQUIRE(s11 == "23");
		StringView s2 = s.mid(0, 50);
		REQUIRE(s2.length() == 10);
		StringView s3 = s.mid(3, 50);
		REQUIRE(s3.length() == 7);
		StringView s31 = s.mid(9, 50);
		REQUIRE(s31 == "0");
		StringView s32 = s.mid(10);
		REQUIRE(s32 == "");
		StringView s4 = s.mid(30, 50);
		REQUIRE(s4.empty());
	}
	DOCTEST_SUBCASE("StringView::getToken()")
	{
		string str("///foo/bar//baz");
		StringView s(str);
		StringView s1 = s.getToken('/');
		REQUIRE(s1.length() == 0);
		s1 = s.mid(s1.end()+1).getToken('/');
		REQUIRE(s1.length() == 0);
		s1 = s.mid(s1.end()+1).getToken('/');
		REQUIRE(s1.length() == 0);
		s1 = s.mid(s1.end()+1).getToken('/');
		REQUIRE(s1 == "foo");
	}
	DOCTEST_SUBCASE("StringView::split()")
	{
		DOCTEST_SUBCASE("Single string")
		{
			string str("a:");
			StringView s(str);
			vector<StringView> sl = s.split(':', StringView::KeepEmptyParts);
			REQUIRE(sl.size() == 2);
			REQUIRE(sl[0] == "a");
			REQUIRE(sl[1].empty());
		}
		DOCTEST_SUBCASE("Quoted string")
		{
			string str("a:\"b:b\":c");
			StringView s(str);
			vector<StringView> sl = s.split(':', '"', StringView::KeepEmptyParts);
			REQUIRE(sl.size() == 3);
			REQUIRE(sl[0] == "a");
			REQUIRE(sl[1] == "\"b:b\"");
			REQUIRE(sl[2] == "c");
		}
		DOCTEST_SUBCASE("Consequent separators")
		{
			string str("///foo/bar//baz//");
			StringView s(str);
			vector<StringView> sl1 = s.split('/');
			REQUIRE(sl1.size() == 3);
			REQUIRE(sl1[1] == "bar");
			vector<StringView> sl2 = s.split('/', StringView::KeepEmptyParts);
			REQUIRE(sl2.size() == 9);
			REQUIRE(sl2[5].empty());
			REQUIRE(sl2[6] == "baz");
		}
	}
}
