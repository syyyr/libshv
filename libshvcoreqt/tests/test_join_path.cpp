#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/coreqt/utils.h>

#include <doctest/doctest.h>

doctest::String toString(const QString& str) {
	return str.toStdString().c_str();
}

DOCTEST_TEST_CASE("joinPath")
{
	using shv::coreqt::Utils;

	QString prefix;
	QString suffix;
	QString result;

	DOCTEST_SUBCASE("regular")
	{
		prefix = "foo";
		suffix = "bar";
		result = "foo/bar";
	}

	DOCTEST_SUBCASE("trailing slash left operand")
	{
		prefix = "foo/";
		suffix = "bar";
		result = "foo/bar";
	}

	DOCTEST_SUBCASE("trailing slash right operand")
	{
		prefix = "foo";
		suffix = "bar/";
		result = "foo/bar/";
	}

	DOCTEST_SUBCASE("trailing slash both operands")
	{
		prefix = "foo/";
		suffix = "bar/";
		result = "foo/bar/";
	}

	DOCTEST_SUBCASE("left operand empty")
	{
		prefix = "";
		suffix = "bar";
		result = "bar";
	}

	DOCTEST_SUBCASE("right operand empty")
	{
		prefix = "foo";
		suffix = "";
		result = "foo";
	}

	DOCTEST_SUBCASE("leading slash left operand")
	{
		prefix = "/foo";
		suffix = "bar";
		result = "/foo/bar";
	}

	DOCTEST_SUBCASE("leading slash right operand")
	{
		prefix = "foo";
		suffix = "/bar";
		result = "foo/bar";
	}

	REQUIRE(Utils::joinPath(prefix, suffix) == result);
}

