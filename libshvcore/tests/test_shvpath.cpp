#include <shv/core/utils/shvpath.h>
#include <shv/core/stringview.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace shv::core::utils;
using namespace shv::core;
using namespace std;

DOCTEST_TEST_CASE("ShvPath")
{
	DOCTEST_SUBCASE("ShvPath::startsWithPath()")
	{
		{
			string path = "status/errorCommunication";
			string paths[] = {
				"status/errorCommunication",
				"status/errorCommunication/",
				"status/errorCommunication/foo",
				"status/errorCommunication/foo/",
			};
			for(const auto &p : paths) {
				REQUIRE(ShvPath::startsWithPath(p, path));
			}
		}
		{
			string path = "foo/bar";
			string paths[] = {
				"bar/baz",
				"foo/barbaz",
			};
			for(const auto &p : paths) {
				REQUIRE(!ShvPath::startsWithPath(p, path));
			}
		}
	}
	DOCTEST_SUBCASE("ShvPath::firstDir()")
	{
		string dir = "'a/b/c'/d";
		string dir1 = "a/b/c";
		REQUIRE(ShvPath::firstDir(dir) == StringView(dir1));
	}
	DOCTEST_SUBCASE("ShvPath::takeFirsDir()")
	{
		string dir = "'a/b/c'/d";
		StringView dir_view(dir);
		string dir1 = "a/b/c";
		string dir2 = "d";
		auto dir_view1 = ShvPath::takeFirsDir(dir_view);
		REQUIRE(dir_view1 == StringView(dir1));
		REQUIRE(dir_view == StringView(dir2));
	}
	DOCTEST_SUBCASE("ShvPath join & split")
	{
		string dir1 = "foo";
		string dir2 = "bar/baz";
		string joined = "foo/'bar/baz'";

		DOCTEST_SUBCASE("ShvPath::joinDirs()")
		{
			REQUIRE(ShvPath::joinDirs(dir1, dir2) == joined);
		}
		DOCTEST_SUBCASE("ShvPath::splitPath()")
		{
			StringViewList split{dir1, dir2};
			REQUIRE(ShvPath::splitPath(joined) == split);
		}
	}
	DOCTEST_SUBCASE("ShvPath::forEachDirAndSubdirs()")
	{
		using Map = std::map<std::string, std::string>;
		Map m {
			{"A/b", "a"},
			{"a/b", "a"},
			{"a/b-c", "a"},
			{"a/b/c", "a"},
			{"a/b/c/d", "a"},
			{"a/b/d", "a"},
			{"a/c/d", "a"},
		};
		ShvPath::forEachDirAndSubdirs(m, "a/b", [](Map::const_iterator it) {
			REQUIRE(ShvPath::startsWithPath(it->first, string("a/b")));
		});
	}
}
