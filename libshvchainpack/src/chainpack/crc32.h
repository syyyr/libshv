#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace shv::chainpack {

using crc32_t = uint32_t;
constexpr crc32_t CRC_POSIX_POLY = 0x04C11DB7;

template<crc32_t POLY>
class Crc32
{
	static constexpr crc32_t WIDTH = 8 * sizeof(crc32_t);
	static constexpr crc32_t TOPBIT = 1 << (WIDTH - 1);
public:

	constexpr Crc32() {
		crc32_t  remainder;
		for (int dividend = 0; dividend < 256; ++dividend)
		{
			// Start with the dividend followed by zeros.
			remainder = dividend << (WIDTH - 8);
			//Perform modulo-2 division, a bit at a time.
			for (uint8_t bit = 8; bit > 0; --bit) {
				//Try to divide the current data bit.
				if (remainder & TOPBIT) {
					remainder = (remainder << 1) ^ POLY;
				}
				else {
					remainder = (remainder << 1);
				}
			}
			//Store the result into the table.
			m_table[dividend] = static_cast<uint8_t>(remainder);
		}
	}

	void add(uint8_t b) {
		auto data = b ^ (m_remainder >> (WIDTH - 8));
		m_remainder = m_table[data] ^ (m_remainder << 8);
		//m_byteCount++;
	}
	void add(const char *data, size_t n) {
		for (size_t i = 0; i < n; ++i) {
			add(static_cast<uint8_t>(data[i]));
		}
	}
	void add(const uint8_t *data, size_t n) {
		for (size_t i = 0; i < n; ++i) {
			add(data[i]);
		}
	}
	crc32_t remainder() const { return m_remainder; }
	//size_t byteCount() const { return m_byteCount; }
private:
	crc32_t m_remainder = 0;
	//size_t m_byteCount = 0;
	uint8_t m_table[256];
};

using Crc32Posix = Crc32<CRC_POSIX_POLY>;

}
