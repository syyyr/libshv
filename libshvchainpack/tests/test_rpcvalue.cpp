#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/rpcvalue.h>

#include <necrolog.h>

#include <doctest/doctest.h>

#include <optional>

using namespace shv::chainpack;
using std::string;

// Check that ChainPack has the properties we want.
#define CHECK_TRAIT(x) static_assert(std::x::value, #x)
CHECK_TRAIT(is_nothrow_constructible<RpcValue>);
CHECK_TRAIT(is_nothrow_default_constructible<RpcValue>);
CHECK_TRAIT(is_copy_constructible<RpcValue>);
CHECK_TRAIT(is_move_constructible<RpcValue>);
CHECK_TRAIT(is_copy_assignable<RpcValue>);
CHECK_TRAIT(is_move_assignable<RpcValue>);
CHECK_TRAIT(is_nothrow_destructible<RpcValue>);

namespace {

template< typename T >
std::string int_to_hex( T i )
{
	std::stringstream stream;
	stream << "0x"
			  //<< std::setfill ('0') << std::setw(sizeof(T)*2)
		   << std::hex << i;
	return stream.str();
}

}

namespace shv::chainpack {

doctest::String toString(const RpcValue& value) {
	return value.toCpon().c_str();
}
/*
doctest::String toString(const RpcValue::List& value) {
	return RpcValue(value).toCpon().c_str();
}

doctest::String toString(const RpcValue::Map& value) {
	return RpcValue(value).toCpon().c_str();
}
*/
}

DOCTEST_TEST_CASE("RpcValue")
{

	DOCTEST_SUBCASE("refcnt test")
	{
		nDebug() << "================================= RefCnt Test =====================================";
		auto rpcval = RpcValue::fromCpon(R"(<1:2,2:12,8:"foo",9:[1,2,3],"bar":"baz",>{"META":17,"18":19})");
		auto rv1 = rpcval;
		//nDebug() << "rv1:" << rv1.toCpon().c_str();
		REQUIRE(rpcval.refCnt() == 2);
		{
			auto rv2 = rpcval;
			REQUIRE(rpcval.refCnt() == 3);
			REQUIRE(rpcval == rv2);
		}
		REQUIRE(rpcval.refCnt() == 2);
		rv1.set("foo", "bar");
		REQUIRE(rv1.refCnt() == 1);
		REQUIRE(rpcval.refCnt() == 1);
		//REQUIRE(rpcval.at("foo") == RpcValue(42));
		REQUIRE(rv1.at("foo") == RpcValue("bar"));
		rv1 = rpcval;
		REQUIRE(rpcval.refCnt() == 2);
		rpcval.setMetaValue(2, 42);
		REQUIRE(rv1.refCnt() == 1);
		REQUIRE(rpcval.refCnt() == 1);
		REQUIRE(rpcval.metaValue(2) == RpcValue(42));
		REQUIRE(rv1.metaValue(2) == RpcValue(12));
	}
	DOCTEST_SUBCASE("strip Meta test")
	{
		nDebug() << "================================= stripMeta Test =====================================";
		auto rpcval = RpcValue::fromCpon(R"(<1:2,2:12,8:"foo",9:[1,2,3],"bar":"baz",>{"META":17,"18":19})");
		auto rv1 = rpcval;
		auto rv2 = rv1;
		auto rv3 = rv1.metaStripped();
		//nDebug() << "rv1:" << rv1.toCpon().c_str();
		REQUIRE(rpcval.refCnt() == 3);
		REQUIRE(rv3.refCnt() == 1);
		REQUIRE(rv1.metaData().isEmpty() == false);
		REQUIRE(rv3.metaData().isEmpty() == true);
		REQUIRE(rv3.at("18") == rpcval.at("18"));
	}
}
