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
	DOCTEST_SUBCASE("pattern match")
	{
		struct Test {
			std::string path;
			std::string pattern;
			bool result;
		};
		Test cases[] {
			{"", "aa", false},
			{"aa", "", false},
			{"", "", true},
			{"", "**", true},
			{"aa/bb/cc/dd", "aa/*/cc/dd", true},
			{"aa/bb/cc/dd", "aa/bb/**/cc/dd", true},
			{"aa/bb/cc/dd", "aa/bb/*/**/dd", true},
			{"aa/bb/cc/dd", "*/*/cc/dd", true},
			{"aa/bb/cc/dd", "*/cc/dd/**", false},
			{"aa/bb/cc/dd", "*/*/*/*", true},
			{"aa/bb/cc/dd", "aa/*/**", true},
			{"aa/bb/cc/dd", "aa/**/dd", true},
			{"aa/bb/cc/dd", "**", true},
			{"aa/bb/cc/dd", "**/dd", true},
			{"aa/bb/cc/dd", "**/*/**", false},
			{"aa/bb/cc/dd", "**/**", false},
			{"aa/bb/cc/dd", "**/ddd", false},
			{"aa/bb/cc/dd", "aa1/bb/cc/dd", false},
			{"aa/bb/cc/dd", "aa/bb/cc/dd1", false},
			{"aa/bb/cc/dd", "aa/bb/cc1/dd", false},
			{"aa/bb/cc/dd", "bb/cc/dd", false},
			{"aa/bb/cc/dd", "aa/bb/cc", false},
			{"aa/bb/cc/dd", "aa/*/bb/cc", false},
			{"aa/bb/cc/dd", "aa/bb/cc/dd/*", false},
			{"aa/bb/cc/dd", "*/aa/bb/cc/dd", false},
			{"aa/bb/cc/dd", "*/aa/bb/cc/dd/*", false},
			{"aa/bb/cc/dd", "aa/bb/cc/dd/ee", false},
			{"aa/bb/cc/dd", "aa/bb/cc", false},
			{"aa/bb/cc/dd", "*/*/*", false},
			{"aa/bb/cc/dd", "*/*/*/*/*", false},
			{"aa/bb/cc/dd", "*/**/*/*/*", false},
		};
		for(const Test &t : cases) {
			//nDebug() << t.path << "vs. pattern" << t.pattern << "should be:" << t.result;
			REQUIRE(shv::core::utils::ShvPath(t.path).matchWild(t.pattern) == t.result);
		}
	}

}
