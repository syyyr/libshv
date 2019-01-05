"use strict"

class ChainPack
{
	static unpackUInt(unpack_context)
	{
		let bitlen = 0;
		let num = 0;
		let ix = 0;

		let head = unpack_context.getByte();
		let bytes_to_read_cnt;
		if     ((head & 128) === 0) {bytes_to_read_cnt = 0; num = head & 127; bitlen = 7;}
		else if((head &  64) === 0) {bytes_to_read_cnt = 1; num = head & 63; bitlen = 6 + 8;}
		else if((head &  32) === 0) {bytes_to_read_cnt = 2; num = head & 31; bitlen = 5 + 2*8;}
		else if((head &  16) === 0) {bytes_to_read_cnt = 3; num = head & 15; bitlen = 4 + 3*8;}
		else {
			bytes_to_read_cnt = (head & 0xf) + 4;
			bitlen = bytes_to_read_cnt * 8;
		}

		for (let i=0; i < bytes_to_read_cnt; i++) {
			let r = unpack_context.getByte();
			num = (num << 8) + r;
		}

		return num;
	}
	/*
	 0 ...  7 bits  1  byte  |0|s|x|x|x|x|x|x|<-- LSB
	 8 ... 14 bits  2  bytes |1|0|s|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
	15 ... 21 bits  3  bytes |1|1|0|s|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
	22 ... 28 bits  4  bytes |1|1|1|0|s|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
	29+       bits  5+ bytes |1|1|1|1|n|n|n|n| |s|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| ... <-- LSB
											n ==  0 ->  4 bytes number (32 bit number)
											n ==  1 ->  5 bytes number
											n == 14 -> 18 bytes number
											n == 15 -> for future (number of bytes will be specified in next byte)
	*/

	// return max bit length >= bit_len, which can be encoded by same number of bytes
	static expandBitLen(bit_len)
	{
		let ret;
		let byte_cnt = ChainPack.bytesNeeded(bit_len);
		if(bit_len <= 28) {
			ret = byte_cnt * (8 - 1) - 1;
		}
		else {
			ret = (byte_cnt - 1) * 8 - 1;
		}
		return ret;
	}

	static packIntData(pack_context, snum)
	{
		let num = snum < 0? -snum: snum;
		let neg = (snum < 0);

		let bitlen = ChainPack.significantBitsPartLength(num);
		bitlen++; // add sign bit
		if(neg) {
			let sign_pos = ChainPack.expandBitLen(bitlen);
			let sign_bit_mask = 1 << sign_pos;
			num |= sign_bit_mask;
		}
		ChainPack.packUIntDataHelper(pack_context, num, bitlen);
	}

	static packUInt(pack_context, n)
	{
		if(n < 64) {
			pack_context.putByte(n % 64);
		}
		else {
			pack_context.putByte(ChainPack.CP_UInt);
			ChainPack.packUIntData(pack_context, n);
		}
	}
}