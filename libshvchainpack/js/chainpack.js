"use strict"

function ChainPack = function()
{
}

ChainPack.ProtocolType = 1;

function ChainPackReader = function(unpack_context)
{
	this.ctx = unpack_context;
}

ChainPackReader.readUInt = function()
{
	let bitlen = 0;
	let num = 0;
	let ix = 0;

	let head = this.cxt.getByte();
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
		let r = this.cxt.getByte();
		num = (num << 8) + r;
	}

	return num;
}

function ChainPackWriter()
{
	this.ctx = new PackContext();
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

// number of bytes needed to encode bit_len
ChainPackWriter.bytesNeeded = function(bit_len)
{
	let cnt;
	if(bit_len <= 28)
		cnt = (bit_len - 1) / 7 + 1;
	else
		cnt = (bit_len - 1) / 8 + 2;
	return cnt;
}

ChainPackWriter.expandBitLen = function(bit_len)
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

ChainPackWriter.prototype.writeUIntDataHelper = function(num, bit_len)
{
	let byte_cnt = ChainPackWriter.bytesNeeded(bit_len);
	let bytes = new Uint8Array(byte_cnt);
	for (let i = byte_cnt-1; i >= 0; --i) {
		let r = num % 256;
		bytes[i] = r;
		num = num / 256;
	}

	let head = 0;
	if(bit_len <= 28) {
		let mask = 0xf0 << (4 - byte_cnt);
		head = head & ~mask;
		mask <<= 1;
		head = head | mask;
	}
	else {
		head = 0xf0 | (byte_cnt - 5);
	}
	bytes[0] = head;

	for (let i = 0; i < byte_cnt; ++i) {
		let r = bytes[i];
		this.ctx.putByte(r);
	}
}

ChainPackWriter.prototype.writeIntData = function(snum)
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
	this.writeUIntDataHelper(num, bitlen);
}

ChainPackWriter.prototype.writeUInt = function(n)
{
	if(n < 64) {
		this.ctx.putByte(n % 64);
	}
	else {
		this.ctx.putByte(ChainPack.CP_UInt);
		this.writeUIntData(pack_context, n);
	}
}

ChainPackWriter.prototype.writeInt = function(n)
{
	if(n >= 0 && n < 64) {
		this.ctx.putByte((n % 64) + 64);
	}
	else {
		this.ctx.putByte(CP_Int);
		this.writeIntData(n);
	}
}