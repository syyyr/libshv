#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/shvtypeinfo.h>
#include <shv/core/log.h>

#define DOCTEST_CONFIG_IMPLEMENT
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
		shvInfo() << "writing file: " << fn;
		CponWriterOptions opts;
		opts.setIndent("\t");
		CponWriter wr(out, opts);
		wr << log;
	}
	else {
		throw runtime_error("cannot open file: " + fn + " for writing");
	}
}

static const std::string FILES_DIR = DEF_FILES_DIR;
static const std::string TEST_DIR = "test_ShvTypeInfo";

int main(int argc, char** argv)
{
	auto tmp_path = fs::temp_directory_path().string();
	fs::current_path(tmp_path);
	fs::remove_all(TEST_DIR);
	fs::create_directory(TEST_DIR);

	return doctest::Context(argc, argv).run();
}

DOCTEST_TEST_CASE("ShvTypeInfo")
{
	auto tmp_path = fs::temp_directory_path().string();
	auto out_path = tmp_path + '/' + TEST_DIR;

	DOCTEST_SUBCASE("nodesTree to typeInfo conversion")
	{
		auto rv = read_cpon_file(FILES_DIR + "/nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(out_path + "/typeInfo.cpon", type_info.toRpcValue());

		REQUIRE(type_info.nodeDescriptionForDevice("TC_G3", "").dataValue("deviceType") == "TC_G3");
		REQUIRE(type_info.nodeDescriptionForDevice("Zone_G3", "").dataValue("deviceType") == "Zone_G3");
		REQUIRE(type_info.nodeDescriptionForDevice("TC_G3", "status").typeName() == "StatusTC");
		REQUIRE(type_info.nodeDescriptionForDevice("Route_G3", "status").typeName() == "StatusRoute");
		REQUIRE(type_info.nodeDescriptionForPath("devices/zone/langevelden/route/AB/status").typeName() == "StatusRoute");
		REQUIRE(type_info.nodeDescriptionForPath("devices/zone/langevelden/route/DC/status").typeName() == "StatusRoute");
		REQUIRE(type_info.nodeDescriptionForDevice("SignalSymbol_G3", "status").typeName() == "StatusSignalSymbol");
		REQUIRE(type_info.nodeDescriptionForPath("devices/signal/SA04/symbol/WHITE/status").typeName() == "StatusSignalSymbolWhite");
		REQUIRE(type_info.nodeDescriptionForPath("devices/signal/SG01/symbol/P80Y/status").typeName() == "StatusSignalSymbolP80Y");
		{
			// extra tags
			auto et = type_info.extraTagsForPath("devices/tc/TC04");
			REQUIRE(et.asMap().value("brclab").asMap().value("url").asString() == "brclab://192.168.1.10:4000/4");
		}
	}

	DOCTEST_SUBCASE("Invalid result for nodeDescriptionForPath(...) function")
	{
		auto rv = read_cpon_file(FILES_DIR + "/hel002_nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(out_path + "/hel002_typeInfo.cpon", type_info.toRpcValue());
		string field_name;

		//DOCTEST_SUBCASE("Node descriptions")
		{
			auto nd = type_info.nodeDescriptionForPath("heating/group/ESHS_Z1/status/errorAutomaticControl", &field_name);
			INFO("nodeDescriptionForPath: ", nd.toRpcValue().toCpon());
			//CAPTURE(nd.toRpcValue().toCpon());
			REQUIRE(nd.isValid() == true);
			REQUIRE(nd.typeName() == "StatusHeatingGroupNew");
			REQUIRE(field_name == "errorAutomaticControl");
		}
		//DOCTEST_SUBCASE("System paths")
		{
			REQUIRE(type_info.systemPathsRoots().empty() == false);
			REQUIRE(type_info.findSystemPath("a/b/c") == "system/sig");
			REQUIRE(type_info.findSystemPath("elbox/VL1-1") == "system/eshs");
			REQUIRE(type_info.findSystemPath("elbox/VL1-1/status") == "system/eshs");
		}
		{
			// extra tags should not be empty
			for(const auto &[key, val] : type_info.extraTags()) {
				REQUIRE(!val.asMap().empty());
			}
		}
	}

	DOCTEST_SUBCASE("Blacklists")
	{
		auto rv = read_cpon_file(FILES_DIR + "/ghe022_nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(out_path + "/ghe022_typeInfo.cpon", type_info.toRpcValue());
		string field_name;

		{
			// extra tags shall contain 'blacklist' key
			bool blacklist_found = false;
			for(const auto &[key, val] : type_info.extraTags()) {
				if(val.asMap().hasKey("blacklist")) {
					blacklist_found = true;
					break;
				}
			}
			REQUIRE(blacklist_found);
		}
		CAPTURE(type_info.extraTagsForPath("devices/spie/TDI").toCpon());
		REQUIRE(type_info.extraTagsForPath("devices/spie/TDI").asMap().value("blacklist").asMap() == RpcValue::Map{{"status/errorCommunication", nullptr}});
	}
}
