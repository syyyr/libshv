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
/*
crc32_t crc32_for_byte(uint8_t b)
{
	constexpr crc32_t CRC_POSIX_POLY_REV = 0xEDB88320L;
	constexpr auto MSB_MASK = static_cast<crc32_t>(0xFF) << ((sizeof(crc32_t) - 1) * 8);
	//constexpr crc32_t MSB_MASK = 0xFF000000L;
	auto r = static_cast<crc32_t>(b);
	for(int j = 0; j < 8; ++j)
		r = (r & 1? 0: CRC_POSIX_POLY_REV) ^ (r >> 1);
	return r ^ MSB_MASK;
}

crc32_t crc32(const void *data, size_t n_bytes)
{
	static uint32_t table[0x100];
	if(!*table)
		for(crc32_t i = 0; i < 0x100; ++i)
			table[i] = crc32_for_byte(static_cast<uint8_t>(i));
	crc32_t crc = 0;
	for(size_t i = 0; i < n_bytes; ++i)
		crc = table[static_cast<uint8_t>(crc ^ static_cast<const uint8_t*>(data)[i])] ^ (crc >> 8);
	return crc;
}
*/
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

	//nInfo() << "POLY:" << utils::intToHex(CRC_POSIX_POLY_REV);
	//nInfo() << "zip:" << utils::intToHex(zip_crc);
	//nInfo() << "crc:" << utils::intToHex(my_crc);
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

