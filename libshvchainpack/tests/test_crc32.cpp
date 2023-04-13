#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <shv/chainpack/crc32.h>
#include <shv/chainpack/utils.h>

#include <necrolog.h>

#include <doctest/doctest.h>

#include <zlib.h>

using namespace shv::chainpack;
using namespace std;
using namespace std::string_literals;

DOCTEST_TEST_CASE("CRC on CRC_POSIX_POLY")
{
	constexpr auto N = sizeof(CRC_POSIX_POLY_REV);
	array<uint8_t, N> arr;
	for(size_t i = 0 ; i < N; ++i) {
		arr[i] = static_cast<uint8_t>(CRC_POSIX_POLY_REV >> (i * 8));
	}
	auto zip_crc = crc32(0L, Z_NULL, 0);
	auto data = reinterpret_cast<const unsigned char*>(arr.data());
	zip_crc = crc32(zip_crc, data, arr.size());

	shv::chainpack::Crc32Posix crc;
	crc.add(arr.data(), arr.size());
	auto my_crc = crc.remainder();

	REQUIRE(zip_crc == my_crc);
}
DOCTEST_TEST_CASE("CRC on strings")
{
	for(const auto &s : {
		""s,
		"a"s,
		"ab"s,
		"auto data = reinterpret_cast<const unsigned char*>(s.data());"s,
		"\0\0\0\0\0\0\0\0\0\0\0\0"s,
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

