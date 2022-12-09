#include <shv/coreqt/rpc.h>
#include <shv/coreqt/log.h>

#include <QVariant>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

using namespace shv::coreqt::rpc;
using namespace shv::chainpack;
using namespace std;

DOCTEST_TEST_CASE("RpcValue to QVariant conversion and vice versa")
{
	DOCTEST_SUBCASE("Map")
	{
		RpcValue rv1 = RpcValue::Map {
			{"foo", 1},
			{"bar", "baz"},
		};
		QVariant qv = rpcValueToQVariant(rv1);
		auto rv2 = qVariantToRpcValue(qv);
		REQUIRE(rv1 == rv2);
	}
	DOCTEST_SUBCASE("IMap")
	{
		RpcValue rv1 = RpcValue::IMap {
			{2, 1},
			{-1, "baz"},
		};
		QVariant qv = rpcValueToQVariant(rv1);
		//shvInfo() << "qv:" << qv.type() << qv.typeName();
		auto rv2 = qVariantToRpcValue(qv);
		REQUIRE(rv1 == rv2);
	}
}
