#include "chainpacktokenizer.h"

namespace shv {
namespace chainpack {

template<typename T>
T readData_UInt(std::istream &data, int *pbitlen = nullptr)
{
	T num = 0;
	int bitlen = 0;
	do {
		if(data.eof())
			break;
		uint8_t head = data.get();

		int bytes_to_read_cnt;
		if     ((head & 128) == 0) {bytes_to_read_cnt = 0; num = head & 127; bitlen = 7;}
		else if((head &  64) == 0) {bytes_to_read_cnt = 1; num = head & 63; bitlen = 6 + 8;}
		else if((head &  32) == 0) {bytes_to_read_cnt = 2; num = head & 31; bitlen = 5 + 2*8;}
		else if((head &  16) == 0) {bytes_to_read_cnt = 3; num = head & 15; bitlen = 4 + 3*8;}
		else {
			bytes_to_read_cnt = (head & 0xf) + 4;
			bitlen = bytes_to_read_cnt * 8;
		}

		for (int i = 0; i < bytes_to_read_cnt; ++i) {
			if(data.eof()) {
				bitlen = 0;
				break;
			}
			uint8_t r = data.get();
			num = (num << 8) + r;
		};
	} while(false);
	if(pbitlen)
		*pbitlen = bitlen;
	return num;
}

uint64_t ChainPackTokenizer::readUIntData(std::istream &in, bool *ok)
{
	int bitlen;
	uint64_t ret = readData_UInt<uint64_t>(in, &bitlen);
	if(ok)
		*ok = (bitlen > 0);
	return ret;
}

} // namespace chainpack
} // namespace shv
