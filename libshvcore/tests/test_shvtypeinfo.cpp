#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/shvtypeinfo.h>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <optional>
#include <filesystem>

namespace fs = std::filesystem;
using namespace shv::chainpack;
using namespace shv::core::utils;
using namespace std;

RpcValue read_cpon_file(const string &fn)
{
	ifstream in(fn, std::ios::binary | std::ios::in);
	if(in) {
		CponReader rd(in);
		return rd.read();
	}
	else {
		throw runtime_error("cannot open file: " + fn + " for reading");
	}
}

void write_cpon_file(const string &fn, const RpcValue &log)
{
	ofstream out(fn, std::ios::binary | std::ios::out);
	if(out) {
		CponWriterOptions opts;
		opts.setIndent("\t");
		CponWriter wr(out, opts);
		wr << log;
	}
	else {
		throw runtime_error("cannot open file: " + fn + " for writing");
	}
}

DOCTEST_TEST_CASE("ShvTypeInfo")
{
	static const std::string FILES_DIR = DEF_FILES_DIR;
	static const std::string TEST_DIR = "test_ShvTypeInfo";
	auto tmp_path = fs::temp_directory_path().string();
	fs::current_path(tmp_path);
	fs::remove_all(TEST_DIR);
	fs::create_directory(TEST_DIR);

	std::string input;

	DOCTEST_SUBCASE("nodesTree to typeInfo conversion")
	{
		auto rv = read_cpon_file(FILES_DIR + "/nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(TEST_DIR + "/typeInfo.cpon", type_info.toRpcValue());
		REQUIRE(type_info.nodeDescriptionForDevice("TC_G3", "").dataValue("deviceType") == "TC_G3");
		REQUIRE(type_info.nodeDescriptionForDevice("Zone_G3", "").dataValue("deviceType") == "Zone_G3");
		REQUIRE(type_info.nodeDescriptionForDevice("TC_G3", "status").typeName() == "StatusTC");
		REQUIRE(type_info.nodeDescriptionForDevice("Route_G3", "status").typeName() == "StatusRoute");
		REQUIRE(type_info.nodeDescriptionForPath("devices/zone/langevelden/route/AB/status").typeName() == "StatusRoute");
		REQUIRE(type_info.nodeDescriptionForPath("devices/zone/langevelden/route/DC/status").typeName() == "StatusRoute");
		REQUIRE(type_info.nodeDescriptionForDevice("SignalSymbol_G3", "status").typeName() == "StatusSignalSymbol");
		REQUIRE(type_info.nodeDescriptionForPath("devices/signal/SA04/symbol/WHITE/status").typeName() == "StatusSignalSymbolWhite");
		REQUIRE(type_info.nodeDescriptionForPath("devices/signal/SG01/symbol/P80Y/status").typeName() == "StatusSignalSymbolP80Y");
		auto et = type_info.extraTagsForPath("devices/tc/TC04");
		REQUIRE(et.asMap().value("brclab").asMap().value("url").asString() == "brclab://192.168.1.10:4000/4");
	}

	DOCTEST_SUBCASE("Invalid result for nodeDescriptionForPath(...) function")
	{
		auto rv = read_cpon_file(FILES_DIR + "/hel002_nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(TEST_DIR + "/hel002_typeInfo.cpon", type_info.toRpcValue());
		string field_name;
		{
			auto nd = type_info.nodeDescriptionForPath("heating/group/ESHS_Z1/status/errorAutomaticControl", &field_name);
			INFO("nodeDescriptionForPath: ", nd.toRpcValue().toCpon());
			//CAPTURE(nd.toRpcValue().toCpon());
			REQUIRE(nd.isValid() == true);
			REQUIRE(nd.typeName() == "StatusHeatingGroupNew");
			REQUIRE(field_name == "errorAutomaticControl");
		}
	}
}
