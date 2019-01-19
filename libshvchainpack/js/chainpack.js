"use strict"

function ChainPack()
{
}

ChainPack.ProtocolType = 1;

ChainPack.CP_Null = 128;
ChainPack.CP_UInt = 129;
ChainPack.CP_Int = 130;
ChainPack.CP_Double = 131;
ChainPack.CP_Bool = 132;
//ChainPack.CP_Blob_depr; // deprecated
ChainPack.CP_String = 134;
//ChainPack.CP_DateTimeEpoch_depr; // deprecated
ChainPack.CP_List = 136;
ChainPack.CP_Map = 137;
ChainPack.CP_IMap = 138;
ChainPack.CP_MetaMap = 139;
ChainPack.CP_Decimal = 140;
ChainPack.CP_DateTime = 141;
ChainPack.CP_CString = 142;
ChainPack.CP_FALSE = 253;
ChainPack.CP_TRUE = 254;
ChainPack.CP_TERM = 255;

// UTC msec since 2.2. 2018
// Fri Feb 02 2018 00:00:00 == 1517529600 EPOCH
ChainPack.SHV_EPOCH_MSEC = 1517529600000;

ChainPack.isLittleEndian = (function() {
	let buffer = new ArrayBuffer(2);
	new DataView(buffer).setInt16(0, 256, true /* littleEndian */);
	// Int16Array uses the platform's endianness.
	return new Int16Array(buffer)[0] === 256;
})();

ChainPack.div = function(n, d)
{
	let r = n % d;
	if(!Number.isInteger(r))
		throw new RangeError("Number too big for current implementation of DIV function: " + n + " DIV " + d)
	return [(n - r) / d, r];
}

ChainPack.uIntToBBE = function(num)
{
	let bytes = new Uint8Array(24);
	let len = 0;
	while(true) {
		[num, bytes[len++]] = ChainPack.div(num, 256)
		if(num == 0)
			break;
	}
	/*
	for(let i=0; i<len / 2 | 0; i++) {
		[bytes[i], bytes[len-i-1]] = [bytes[len-i-1], bytes[i]];
		//let b = bytes[i]
		//bytes[i] = bytes[len-i-1]
		//bytes[len-i-1] = b
	}
	*/
	bytes = bytes.subarray(0, len)
	bytes.reverse();
	return bytes
}

ChainPack.rotateLeftBBE = function(bytes, cnt)
{
	let nbytes = new Uint8Array(bytes.length)
	nbytes.set(bytes)
	let is_neg = nbytes[0] & 128;

	for(let j=0; j<cnt; j++) {
		let cy = 0;
		for(let i=nbytes.length - 1; i >= 0; i--) {
			let cy1 = nbytes[i] & 128;
			nbytes[i] <<= 1;
			if(cy)
				nbytes[i] |= 1
			cy = cy1
		}
		if(cy) {
			// prepend byte
			let nbytes2 = new Uint8Array(nbytes.length + 1)
			nbytes2.set(nbytes, 1);
			nbytes = nbytes2
			nbytes[0] = 1
		}
	}
	if(is_neg) for(let i=0; i<cnt; i++) {
		let mask = 128;
		for(let j = 0; j < 8; j++) {
			if(nbytes[i] & mask)
				return nbytes;
			nbytes[i] |= mask;
			mask >>= 1;
		}
	}
	return nbytes;
}

function ChainPackReader(unpack_context)
{
	if(unpack_context.constructor.name === "ArrayBuffer")
		unpack_context = new UnpackContext(unpack_context)
	else if(unpack_context.constructor.name === "Uint8Array")
		unpack_context = new UnpackContext(unpack_context)
	if(unpack_context.constructor.name !== "UnpackContext")
		throw new TypeError("ChainpackReader must be constructed with UnpackContext")
	this.ctx = unpack_context;
}

ChainPackReader.prototype.read = function()
{
	let rpc_val = new RpcValue();
	let packing_schema = this.ctx.getByte();

	if(packing_schema == ChainPack.CP_MetaMap) {
		rpc_val.meta = this.readMap();
	}

	if(packing_schema < 128) {
		if(packing_schema & 64) {
			// tiny Int
			rpc_val.type = RpcValue.Type.Int;
			rpc_val.value = packing_schema & 63;
		}
		else {
			// tiny UInt
			rpc_val.type = RpcValue.Type.UInt;
			rpc_val.value = packing_schema & 63;
		}
	}
	else {
		switch(packing_schema) {
		case ChainPack.CP_Null: {
			rpc_val.type = RpcValue.Type.Null;
			rpc_val.value = null;
			break;
		}
		case ChainPack.CP_TRUE: {
			rpc_val.type = RpcValue.Type.Bool;
			rpc_val.value = true;
			break;
		}
		case ChainPack.CP_FALSE: {
			rpc_val.type = RpcValue.Type.Bool;
			rpc_val.value = false;
			break;
		}
		case ChainPack.CP_Int: {
			rpc_val.value = this.readUIntData()
			rpc_val.type = RpcValue.Type.Int;
			break;
		}
		case ChainPack.CP_UInt: {
			rpc_val.value = this.readUIntData()
			rpc_val.type = RpcValue.Type.UInt;
			break;
		}
		case ChainPack.CP_Double: {
			let data = new Uint8Array(8);
			for (var i = 0; i < 8; i++)
				data[i] = this.ctx.getByte();
			rpc_val.value = new DataView(dat.buffer).getFloat64(0, true); //little endian
			rpc_val.type = RpcValue.Type.Double;
			break;
		}
		case ChainPack.CP_Decimal: {
			let mant = this.readIntData();
			let exp = this.readIntData();
			rpc_val.value = {mantisa: mant, exponent: exp};
			rpc_val.type = RpcValue.Type.Decimal;
			break;
		}
		case ChainPack.CP_DateTime: {
			let d = this.readUIntData();
			let offset = 0;
			let has_tz_offset = d & 1;
			let has_not_msec = d & 2;
			d >>= 2;
			if(has_tz_offset) {
				offset = d & 0x7F;
				if(offset & 0x40) {
					// sign extension
					offset &= 0x3F;
					offset = -offset;
				}
				d >>= 7;
			}
			if(has_not_msec)
				d *= 1000;
			d += ChainPack.SHV_EPOCH_MSEC;

			rpc_val.value = {epochMsec: d, utcOffsetMin: offset * 15};
			rpc_val.type = RpcValue.Type.DateTime;
			break;
		}
		case ChainPack.CP_Map: {
			rpc_val.value = this.readMap();
			rpc_val.type = RpcValue.Type.Map;
			break;
		}
		case ChainPack.CP_IMap: {
			rpc_val.value = this.readMap();
			rpc_val.type = RpcValue.Type.IMap;
			break;
		}
		case ChainPack.CP_List: {
			rpc_val.value = this.readList();
			rpc_val.type = RpcValue.Type.Map;
			break;
		}
		case ChainPack.CP_String: {
			let str_len = this.readUInt();
			let arr = Uint8Array(str_len)
			for (var i = 0; i < str_len; i++)
				arr[i] = getByte()
			rpc_val.value = arr.buffer;
			rpc_val.type = RpcValue.Type.String;
			break;
		}
		case ChainPack.CP_CString:
		{
			// variation of CponReader.readCString()
			let pctx = new PackContext();
			while(true) {
				let b = this.ctx.getByte();
				if(b == '\\'.charCodeAt(0)) {
					b = this.ctx.getByte();
					switch (b) {
					case '\\'.charCodeAt(0): pctx.putByte("\\"); break;
					case '0'.charCodeAt(0): pctx.putByte(0); break;
					default: pctx.putByte(b); break;
					}
				}
				else {
					if (b == 0) {
						// end of string
						break;
					}
					else {
						pctx.putByte(b);
					}
				}
			}
			rpc_val.value = pctx.buffer();
			rpc_val.type = RpcValue.Type.String;

			break;
		}
		default:
			throw new TypeError("ChainPack - Invalid type info.");
		}
	}
	return rpc_val;
}

ChainPackReader.prototype.readUIntData = function(info)
{
	let bitlen = 0;
	let num = 0;
	let ix = 0;

	let head = this.ctx.getByte();
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
		let r = this.ctx.getByte();
		num = (num << 8) + r;
	}
	if(info)
		info.bitlen = bitlen
	return num;
}

ChainPackReader.prototype.readIntData = function()
{
	let info = {};
	let num = this.readUIntData(info);
	let sign_bit_mask = 1 << (info.bitlen - 1);
	let neg = num & sign_bit_mask;
	let snum = num;
	if(neg) {
		snum &= ~sign_bit_mask;
		snum = -snum;
	}
	return snum;
}

function ChainPackWriter()
{
	this.ctx = new PackContext();
}

ChainPackWriter.prototype.write = function(rpc_val)
{
	if(!(rpc_val && rpc_val.constructor.name === "RpcValue"))
		rpc_val = new RpcValue(rpc_val)
	if(rpc_val && rpc_val.constructor.name === "RpcValue") {
		if(rpc_val.meta) {
			this.writeMeta(rpc_val.meta);
		}
		switch (rpc_val.type) {
		case RpcValue.Type.Null: this.ctx.putByte(ChainPack.CP_Null); break;
		case RpcValue.Type.Bool: this.ctx.putByte(rpc_val.value? ChainPack.CP_TRUE: ChainPack.CP_FALSE); break;
		case RpcValue.Type.String: this.writeString(rpc_val.value); break;
		case RpcValue.Type.UInt: this.writeUInt(rpc_val.value); break;
		case RpcValue.Type.Int: this.writeInt(rpc_val.value); break;
		case RpcValue.Type.Double: this.writeDouble(rpc_val.value); break;
		case RpcValue.Type.Decimal: this.writeDecimal(rpc_val.value); break;
		case RpcValue.Type.List: this.writeList(rpc_val.value); break;
		case RpcValue.Type.Map: this.writeMap(rpc_val.value); break;
		case RpcValue.Type.IMap: this.writeIMap(rpc_val.value); break;
		case RpcValue.Type.DateTime: this.writeDateTime(rpc_val.value); break;
		default:
			// better to write null than create invalid chain-pack
			this.ctx.putByte(ChainPack.CP_Null);
			break;
		}
	}
}

//ChainPackWriter.MAX_BIT_LEN = Math.log(Number.MAX_SAFE_INTEGER) / Math.log(2);
// logcal operator in JS works on 32 bit only
ChainPackWriter.MAX_BIT_LEN = 32;
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

ChainPackWriter.significantBitsPartLength = function(n)
{
	if(n >= 2**31)
		throw new RangeError("Cannot pack int wider than 31 bits")
	const mask = 1 << (ChainPackWriter.MAX_BIT_LEN - 1);
	let len = ChainPackWriter.MAX_BIT_LEN;
	for (; n && !(n & mask); --len) {
		n <<= 1;
	}
	return n? len: 0;
}

// number of bytes needed to encode bit_len
ChainPackWriter.bytesNeeded = function(bit_len)
{
	let cnt;
	if(bit_len <= 28)
		cnt = ((bit_len - 1) / 7 | 0) + 1;
	else
		cnt = ((bit_len - 1) / 8 | 0) + 2;
	return cnt;
}

ChainPackWriter.expandBitLen = function(bit_len)
{
	let ret;
	let byte_cnt = ChainPackWriter.bytesNeeded(bit_len);
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
		num = num / 256 | 0;
	}

	let head = bytes[0];
	if(bit_len <= 28) {
		let mask = (0xf0 << (4 - byte_cnt)) & 0xff;
		head = head & ~mask;
		mask <<= 1;
		mask &= 0xff;
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

ChainPackWriter.prototype.writeUIntData = function(num)
{
	let bitlen = ChainPackWriter.significantBitsPartLength(num);
	this.writeUIntDataHelper(num, bitlen);
}

ChainPackWriter.prototype.writeIntData = function(snum)
{
	let neg = (snum < 0);
	let num = neg? -snum: snum;

	let bitlen = ChainPackWriter.significantBitsPartLength(num);
	bitlen++; // add sign bit
	if(neg) {
		if(bitlen >= 32)
			throw new RangeError("Cannot pack negative int leser than " + (-(2**31 - 1)))
		let sign_pos = ChainPackWriter.expandBitLen(bitlen);
		let sign_bit_mask = 1 << sign_pos;
		num |= sign_bit_mask;
	}
	else {
		if(bitlen >= 33)
			throw new RangeError("Cannot pack int greater than " + (2**32 - 1))
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
		this.writeUIntData(n);
	}
}

ChainPackWriter.prototype.writeInt = function(n)
{
	if(n >= 0 && n < 64) {
		this.ctx.putByte((n % 64) + 64);
	}
	else {
		this.ctx.putByte(ChainPack.CP_Int);
		this.writeIntData(n);
	}
}

ChainPackWriter.prototype.writeDecimal = function(val)
{
	this.ctx.putByte(ChainPack.CP_Decimal);
	this.writeIntData(val.mantisa);
	this.writeIntData(val.exponent);
}

ChainPackWriter.prototype.writeList = function(lst)
{
	this.ctx.putByte(ChainPack.CP_List);
	for(let i=0; i<lst.length; i++)
		this.write(lst[i])
	this.ctx.putByte(ChainPack.CP_TERM);
}

ChainPackWriter.prototype.writeMapData = function(map)
{
	for (let p in map) {
		if (map.hasOwnProperty(p)) {
			let c = p.charCodeAt(0);
			if(c >= 48 && c <= 57) {
				this.writeInt(parseInt(p))
			}
			else {
				this.writeJSString(p);
			}
			this.write(map[p]);
		}
	}
	this.ctx.putByte(ChainPack.CP_TERM);
}

ChainPackWriter.prototype.writeMap = function(map)
{
	this.ctx.putByte(ChainPack.CP_Map);
	this.writeMapData(map);
}

ChainPackWriter.prototype.writeIMap = function(map)
{
	this.ctx.putByte(ChainPack.CP_IMap);
	this.writeMapData(map);
}

ChainPackWriter.prototype.writeMeta = function(map)
{
	this.ctx.putByte(ChainPack.CP_MetaMap);
	this.writeMapData(map);
}

ChainPackWriter.prototype.writeString = function(str)
{
	this.ctx.putByte(ChainPack.CP_String);
	let arr = new Uint8Array(str)
	this.writeUIntData(arr.length)
	for (let i=0; i < arr.length; i++)
		this.ctx.putByte(arr[i])
}

ChainPackWriter.prototype.writeJSString = function(str)
{
	this.ctx.putByte(ChainPack.CP_String);
	let pctx = new PackContext();
	pctx.writeStringUtf8(str);
	this.writeUIntData(pctx.length)
	for (let i=0; i < pctx.length; i++)
		this.ctx.putByte(pctx.data[i])
}
