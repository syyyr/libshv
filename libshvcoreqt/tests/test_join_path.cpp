#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/coreqt/utils.h>

#include <doctest/doctest.h>

doctest::String toString(const QString& str) {
	return str.toStdString().c_str();
}

using namespace shv::coreqt::utils;
DOCTEST_TEST_CASE("joinPath")
{

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

	REQUIRE(joinPath(prefix, suffix) == result);
}

DOCTEST_TEST_CASE("joinPath - variadic arguments")
{
	// The function takes any number of args.
	REQUIRE(joinPath(QString("a")) == "a");
	REQUIRE(joinPath(QString("a"), QString("b")) == "a/b");
	REQUIRE(joinPath(QString("a"), QString("b"), QString("c")) == "a/b/c");
	REQUIRE(joinPath(QString("a"), QString("b"), QString("c"), QString("d")) == "a/b/c/d");
	REQUIRE(joinPath(QString("a"), QString("b"), QString("c"), QString("d"), QString("e")) == "a/b/c/d/e");

	// The function supports std::string.
	REQUIRE(joinPath(QString("a"), std::string("b")) == "a/b");
	REQUIRE(joinPath(std::string("a"), QString("b")) == "a/b");
	REQUIRE(joinPath(std::string("a"), QString("b"), std::string("c")) == "a/b/c");
	REQUIRE(joinPath(QString("a"), std::string("b"), QString("c")) == "a/b/c");
	REQUIRE(joinPath(QString("a"), QString("b"), std::string("c")) == "a/b/c");

	// The function supports character literals.
	REQUIRE(joinPath("a") == "a");
	REQUIRE(joinPath("a", "b") == "a/b");
	REQUIRE(joinPath(QString("a"), "b") == "a/b");
	REQUIRE(joinPath("a", std::string("b")) == "a/b");

	// The function can return either a QString or an std::string.
	REQUIRE(joinPath<QString>("a", "b") == "a/b");
	REQUIRE(joinPath<QString>("a", std::string("b")) == "a/b");
	REQUIRE(joinPath<QString>(QString("a"), "b") == "a/b");
	REQUIRE(joinPath<QString>(QString("a"), std::string("b")) == "a/b");
	REQUIRE(joinPath<QString>(std::string("a"), QString("b")) == "a/b");
	REQUIRE(joinPath<QString>(std::string("a"), std::string("b")) == "a/b");
	REQUIRE(joinPath<std::string>("a", "b") == "a/b");
	REQUIRE(joinPath<std::string>("a", std::string("b")) == "a/b");
	REQUIRE(joinPath<std::string>(QString("a"), "b") == "a/b");
	REQUIRE(joinPath<std::string>(std::string("a"), QString("b"), std::string("c")) == "a/b/c");
	REQUIRE(joinPath<std::string>(std::string("a"), QString("b"), QString("c")) == "a/b/c");
	REQUIRE(joinPath<std::string>(std::string("a"), std::string("b")) == "a/b");
	// QString::toStdString can still be used manually, however, the function will be less efficient about conversions
	// between the string types.
	REQUIRE(joinPath("a", std::string("b")).toStdString() == "a/b");
}
