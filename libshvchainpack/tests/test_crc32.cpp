#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <shv/chainpack/crc32.h>
#include <shv/chainpack/utils.h>

#include <necrolog.h>

#include <doctest/doctest.h>

#include <zlib.h>

using namespace shv::chainpack;
using namespace std;
using namespace std::string_literals;

namespace shv::chainpack {

/*
doctest::String toString(const RpcValue::Map& value) {
	return RpcValue(value).toCpon().c_str();
}
*/
}

DOCTEST_TEST_CASE("CRC on CRC_POSIX_POLY")
{
	array<uint8_t, sizeof(CRC_POSIX_POLY)> arr;
	for(size_t i = 0; i < arr.size(); ++i) {
		arr[i] = static_cast<uint8_t>(CRC_POSIX_POLY >> (i * 8));
	}
	nInfo() << "POLY:" << utils::hexDump(reinterpret_cast<const char*>(arr.data()), arr.size());
	nInfo() << "POLY:" << utils::intToHex(CRC_POSIX_POLY) << utils::intToHex(CRC_POSIX_POLY);
	auto zip_crc = crc32(0L, Z_NULL, 0);
	auto data = reinterpret_cast<const unsigned char*>(arr.data());
	zip_crc = crc32(zip_crc, data, arr.size());

	shv::chainpack::Crc32Posix crc;
	crc.add(arr.data(), arr.size());
	nInfo() << "zip:" << utils::intToHex(zip_crc) << utils::intToHex(zip_crc);
	nInfo() << "crc:" << utils::intToHex(crc.remainder()) << utils::intToHex(crc.remainder());
	REQUIRE(zip_crc == crc.remainder());
}
DOCTEST_TEST_CASE("CRC on strings")
{
	for(const auto &s : {
		""s,
		"a"s,
		"ab"s,
	}) {
		auto zip_crc = crc32(0L, Z_NULL, 0);
		auto data = reinterpret_cast<const unsigned char*>(s.data());
		zip_crc = crc32(zip_crc, data, static_cast<unsigned int>(s.size()));

		shv::chainpack::Crc32Posix crc;
		crc.add(s.data(), s.size());
		CAPTURE(s);
		REQUIRE(zip_crc == crc.remainder());
	}
}

