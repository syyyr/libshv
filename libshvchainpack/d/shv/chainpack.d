module shv.chainpack;

import shv.rpcvalue;
import std.array;
import std.range.primitives;
import std.conv;
import std.traits;// : isSomeByte;
import std.string;
import std.utf;
import std.exception;

debug {
	import std.stdio : writeln;
}


// UTC msec since 2.2. 2018 folowed by signed UTC offset in 1/4 hour
// Fri Feb 02 2018 00:00:00 == 1517529600 EPOCH
enum long SHV_EPOCH_MSEC = 1517529600000;

enum PackingSchema : ubyte {
	Null = 128,
	UInt,
	Int,
	Double,
	Bool,
	Blob_depr, // deprecated
	String,
	DateTimeEpoch_depr, // deprecated
	List,
	Map,
	IMap,
	MetaMap,
	Decimal,
	DateTime,
	CString,
	FALSE = 253,
	TRUE = 254,
	TERM = 255,
}

// see https://en.wikipedia.org/wiki/Find_first_set#CLZ
static enum ubyte[16] sig_table_4bit =  [ 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 ];

static int significant_bits_part_length(ulong n) pure @safe
{
	int len = 0;

	if (n & 0xFFFFFFFF00000000) {
		len += 32;
		n >>= 32;
	}
	if (n & 0xFFFF0000) {
		len += 16;
		n >>= 16;
	}
	if (n & 0xFF00) {
		len += 8;
		n >>= 8;
	}
	if (n & 0xF0) {
		len += 4;
		n >>= 4;
	}
	len += sig_table_4bit[n];
	return len;
}

// number of bytes needed to encode bit_len
static int bytes_needed(int bit_len) pure @safe
{
	int cnt;
	if(bit_len <= 28)
		cnt = (bit_len - 1) / 7 + 1;
	else
		cnt = (bit_len - 1) / 8 + 2;
	return cnt;
}

/* UInt
 0 ...  7 bits  1  byte  |0|x|x|x|x|x|x|x|<-- LSB
 8 ... 14 bits  2  bytes |1|0|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
15 ... 21 bits  3  bytes |1|1|0|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
22 ... 28 bits  4  bytes |1|1|1|0|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
29+       bits  5+ bytes |1|1|1|1|n|n|n|n| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| ... <-- LSB
                    n ==  0 ->  4 bytes number (32 bit number)
                    n ==  1 ->  5 bytes number
                    n == 14 -> 18 bytes number
                    n == 15 -> for future (number of bytes will be specified in next byte)
*/

void pack_uint_data_helper(R)(ref R out_range, ulong num, int bit_len) @safe
{
	enum BYTE_CNT_MAX = 32;
	int byte_cnt = bytes_needed(bit_len);
	enforce(byte_cnt <= BYTE_CNT_MAX, "Max int byte size" ~ to!string(BYTE_CNT_MAX) ~ "exceeded");
	ubyte[BYTE_CNT_MAX] bytes;
	int i;
	for (i = byte_cnt-1; i >= 0; --i) {
		ubyte r = num & 255;
		bytes[i] = r;
		num = num >> 8;
	}

	ref ubyte head() { return bytes[0]; }
	if(bit_len <= 28) {
		ubyte mask = cast(ubyte) (0xf0 << (4 - byte_cnt));
		head = head & cast(ubyte)~cast(int)mask;
		mask <<= 1;
		head = head | mask;
	}
	else {
		head = cast(ubyte) (0xf0 | (byte_cnt - 5));
	}

	for ( i = 0; i < byte_cnt; ++i) {
		ubyte r = bytes[i];
		out_range.put(r);
	}
}

void pack_uint_data(R)(ref R out_range, ulong num) @safe
{
	//const size_t UINT_BYTES_MAX = 18;
	//if(sizeof(num) > UINT_BYTES_MAX) {
	//	pack_context->err_no = CCPCP_RC_LOGICAL_ERROR;//("writeData_UInt: value too big to write!");
	//	return;
	//}

	int bitlen = significant_bits_part_length(num);
	pack_uint_data_helper!R(out_range, num, bitlen);
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
static int expand_bit_len(int bit_len) @safe
{
	int ret;
	int byte_cnt = bytes_needed(bit_len);
	if(bit_len <= 28) {
		ret = byte_cnt * (8 - 1) - 1;
	}
	else {
		ret = (byte_cnt - 1) * 8 - 1;
	}
	return ret;
}

void pack_int_data(R)(ref R out_range, long snum) @safe
{
	ulong num = snum < 0? -snum: snum;
	bool neg = (snum < 0);

	int bitlen = significant_bits_part_length(num);
	bitlen++; // add sign bit
	if(neg) {
		int sign_pos = expand_bit_len(bitlen);
		ulong sign_bit_mask = (cast(ulong) 1) << sign_pos;
		num |= sign_bit_mask;
	}
	pack_uint_data_helper!R(out_range, num, bitlen);
}

void pack_uint(R)(ref R out_range, ulong i) @safe
{
	if(i < 64) {
		out_range.put(cast(ubyte)(i % 64));
	}
	else {
		out_range.put(PackingSchema.UInt);
		pack_uint_data!R(out_range, i);
	}
}

void pack_int(R)(ref R out_range, long i) @safe
{
	if(i >= 0 && i < 64) {
		out_range.put(cast(ubyte)((i % 64) + 64));
	}
	else {
		out_range.put(PackingSchema.Int);
		pack_int_data!R(out_range, i);
	}
}

struct WriteOptions
{
	bool sortKeys = false;
	//string indent;
	bool writeInvalidAsNull = true;
}

void write(T)(ref T out_range, const ref RpcValue rpcval, WriteOptions opts = WriteOptions()) @safe
{
	void putb(ubyte b) { out_range.put(b); }
	void puts(string s) { foreach(c; s) out_range.put(c); }

	void write_int(long n) @safe
	{
		pack_int(out_range, n);
	}

	void write_uint(ulong n) @safe
	{
		pack_uint(out_range, n);
	}

	void write_double(double n) @safe
	{
		putb(PackingSchema.Double);
		union U {
			ubyte[double.sizeof] data;
			double d;
		}
		U u;
		u.d = n;
		for (int i = 0; i < double.sizeof; i++)
			putb(u.data[i]);
	}

	void write_decimal(RpcValue.Decimal n) @safe
	{
		putb(PackingSchema.Decimal);
		pack_int_data(out_range, n.mantisa);
		pack_int_data(out_range, n.exponent);
	}

	void write_datetime(RpcValue.DateTime dt) @safe
	{
		putb(PackingSchema.DateTime);
		long msecs = dt.msecsSinceEpoch - SHV_EPOCH_MSEC;
		int offset = (dt.utcOffsetMin / 15) & 0x7F;
		long ms = msecs % 1000;
		/*
		debug(chainpack) {
			import std.stdio : writeln;
			writeln("write datetime: ", dt);
			writeln("msecs: ", msecs);
			writeln("offset: ", offset);
			writeln("ms: ", ms);
		}
		*/
		if(ms == 0)
			msecs /= 1000;
		if(offset != 0) {
			msecs <<= 7;
			msecs |= offset;
		}
		msecs <<= 2;
		if(offset != 0)
			msecs |= 1;
		if(ms == 0)
			msecs |= 2;
		pack_int_data(out_range, msecs);
	}

	void write_map(R)(R keys, const ref RpcValue[string] map) @safe
	{
		putb(PackingSchema.Map);
		foreach(k; keys) {
			write_string(k);
			write_rpcval(map[k]);
		}
		putb(PackingSchema.TERM);
	}

	void write_imap(R)(R keys, const ref RpcValue[int] map) @safe
	{
		putb(PackingSchema.IMap);
		foreach(k; keys) {
			write_int(k);
			write_rpcval(map[k]);
		}
		putb(PackingSchema.TERM);
	}

	void write_meta(R)(R keys, const ref RpcValue.Meta map) @safe
	{
		putb(PackingSchema.MetaMap);
		foreach(k; keys) {
			if(k.isIntKey)
				write_int(k.ikey);
			else
				write_string(k.skey);
			write_rpcval(map[k]);
		}
		putb(PackingSchema.TERM);
	}

	T[] sorted_keys(T)(const ref RpcValue[T] m) @safe {
		T[] ret;
		foreach(k; m.byKey())
			ret ~= k;
		import std.algorithm : sort;
		sort(ret);
		return ret;
	}

	void write_cstring(string str) @safe
	{
		putb(PackingSchema.CString);
		foreach (char c; str) {
			switch(c) {
			case '\0':
			case '\\':
				putb('\\');
				putb(c);
				break;
			default:
				putb(c);
			}
		}
	}

	void write_string(string str) @safe
	{
		putb(PackingSchema.String);
		pack_uint_data(out_range, str.length);
		puts(str);
	}

	@safe
	void write_rpcval(const ref RpcValue rpcval)
	{
		auto meta = rpcval.meta;
		if(meta.length > 0) {
			if((opts.sortKeys) != 0)
				write_meta(meta.bySortedKey(), meta);
			else
				write_meta(meta.byKey(), meta);
		}
		final switch (rpcval.type)
		{
			case RpcValue.Type.Map:
				auto map = rpcval.mapNoRef;
				if((opts.sortKeys) != 0)
					write_map(sorted_keys!string(map), map);
				else
					write_map(map.byKey(), map);
				break;
			case RpcValue.Type.IMap:
				auto map = rpcval.imapNoRef;
				if((opts.sortKeys) != 0)
					write_imap(sorted_keys!int(map), map);
				else
					write_imap(map.byKey(), map);
				break;

			case RpcValue.Type.List:
				putb(PackingSchema.List);
				auto lst = rpcval.listNoRef;
				foreach(v; lst) {
					write_rpcval(v);
				}
				putb(PackingSchema.TERM);
				break;
			case RpcValue.Type.String:
				write_string(rpcval.str);
				break;
			case RpcValue.Type.Integer:
				write_int(rpcval.integer);
				break;

			case RpcValue.Type.UInteger:
				write_uint(rpcval.uinteger);
				break;

			case RpcValue.Type.Float:
				write_double(rpcval.floating);
				break;
			case RpcValue.Type.Bool:
				if(rpcval.boolean)
					putb(PackingSchema.TRUE);
				else
					putb(PackingSchema.FALSE);
				break;
			case RpcValue.Type.Decimal:
				write_decimal(rpcval.decimal);
				break;
			case RpcValue.Type.DateTime:
				write_datetime(rpcval.datetime);
				break;
			case RpcValue.Type.Null:
				putb(PackingSchema.Null);
				break;
			case RpcValue.Type.Invalid:
				enforce(opts.writeInvalidAsNull, "Cannot write Invalid RpcValue.");
				putb(PackingSchema.Null);
				break;
		}
	}

	write_rpcval(rpcval);
}

class ChainPackReadException : Exception
{
	this(string msg, int pos = -1) pure nothrow @safe
	{
		if (pos)
			super(text(msg, " (Pos:", pos, ")"));
		else
			super(msg);
	}
}

/// @pbitlen is used to enable same function usage for signed int unpacking
static ulong unpack_uint_data_helper(R)(ref R reader, out int bitlen) @safe
{
	ulong num = 0;

	ubyte head = reader.get_byte();

	int bytes_to_read_cnt;
	if     ((head & 128) == 0) {bytes_to_read_cnt = 0; num = head & 127; bitlen = 7;}
	else if((head &  64) == 0) {bytes_to_read_cnt = 1; num = head & 63; bitlen = 6 + 8;}
	else if((head &  32) == 0) {bytes_to_read_cnt = 2; num = head & 31; bitlen = 5 + 2*8;}
	else if((head &  16) == 0) {bytes_to_read_cnt = 3; num = head & 15; bitlen = 4 + 3*8;}
	else {
		bytes_to_read_cnt = (head & 0xf) + 4;
		bitlen = bytes_to_read_cnt * 8;
	}
	int i;
	for (i = 0; i < bytes_to_read_cnt; ++i) {
		ubyte r = reader.get_byte();
		num = (num << 8) + r;
	}

	return num;
}

static ulong unpack_uint_data(R)(ref R reader) @safe
{
	int bitlen = 0;
	return unpack_uint_data_helper(reader, bitlen);
}

static ulong unpack_uint(R)(ref R reader) @safe
{
	auto b = reader.get_byte();
	enforce(b == PackingSchema.UInt, new ChainPackReadException("Data type must be UInt", reader.count));
	return unpack_uint_data(reader);
}

static long unpack_int_data(R)(ref R reader) @safe
{
	long snum = 0;
	int bitlen;
	ulong num = unpack_uint_data_helper(reader, bitlen);
	ulong sign_bit_mask = (cast(ulong)1) << (bitlen - 1);
	bool neg = (num & sign_bit_mask) != 0;
	snum = cast(long)num;
	if(neg) {
		snum &= ~sign_bit_mask;
		snum = -snum;
	}
	return snum;
}

static long unpack_int(R)(ref R reader) @safe
{
	auto b = reader.get_byte();
	enforce(b == PackingSchema.Int, new ChainPackReadException("Data type must be Int", reader.count));
	return unpack_int_data(reader);
}
/*
mixin template Genreader
{
	struct Reader
	{
		enum short NO_BYTE = -1;

		int count = 0;

		short peek_byte() @safe
		{
			if(input_range.empty())
				return NO_BYTE;
			return input_range.front();
		}

		ubyte get_byte() @safe
		{
			ubyte ret = input_range.front();
			input_range.popFront();
			count++;
			return ret;
		}
	}
}
*/
enum short NO_BYTE = -1;
/+
enum use_nogc_trusted = true;
static if(use_nogc_trusted) {
	// only this does not not allocate
	struct Reader(R)
	{
		int count = 0;
		R *pinput_range;

		this(ref R input_range) @trusted @nogc
		{
			pinput_range = &input_range;
		}
		short peek_byte() @safe
		{
			if((*pinput_range).empty())
				return NO_BYTE;
			return (*pinput_range).front();
		}
		ubyte get_byte() @safe
		{
			ubyte ret = (*pinput_range).front();
			(*pinput_range).popFront();
			debug(chainpack) {
				import std.stdio : writeln;
				writeln("get byte [", count, "] ", ret);
			}
			count++;
			return ret;
		}
	}
}
else {
	struct Reader(R)
	{
		int count = 0;

		short delegate() @safe peek_byte;
		ubyte delegate() @safe get_byte;

		this(ref R input_range) @safe
		{
			short peek_byte() @safe
			{
				if(input_range.empty())
					return NO_BYTE;
				return input_range.front();
			}

			ubyte get_byte() @safe
			{
				ubyte ret = input_range.front();
				input_range.popFront();
				debug(chainpack) {
					import std.stdio : writeln;
					writeln("get byte [", count, "] ", ret);
				}
				count++;
				return ret;
			}

			this.peek_byte = &peek_byte;
			this.get_byte = &get_byte;
		}
	}
}
+/
struct Reader(R)
{
	int count = 0;
	R input_range;

	this(R input_range) @trusted @nogc
	{
		this.input_range = input_range;
	}
	short peek_byte() @safe
	{
		if(input_range.empty())
			return NO_BYTE;
		return input_range.front();
	}
	ubyte get_byte() @safe
	{
		ubyte ret = input_range.front();
		input_range.popFront();
		debug(chainpack) {
			import std.stdio : writeln;
			writeln("get byte [", count, "] ", ret);
		}
		count++;
		return ret;
	}
}

ulong readUIntData(R)(R input_range) @safe
{
	auto reader = Reader!R(input_range);
	int bitlen;
	return unpack_uint_data_helper(reader, bitlen);
}

ulong readUInt(R)(R input_range) @safe
{
	auto reader = Reader!R(input_range);
	return unpack_uint(reader);
}

ulong readInt(R)(R input_range) @safe
{
	auto reader = Reader!R(input_range);
	return unpack_int(reader);
}

RpcValue read(R, int max_depth = -1)(R input_range) @safe
if (isInputRange!R && !isInfinite!R && is(Unqual!(ElementType!R) == ubyte))
{
	int depth = -1;
	//int pos = 0;

	auto reader = Reader!R(input_range);

	auto peek_byte() @safe { return reader.peek_byte(); }
	auto get_byte() @safe { return reader.get_byte(); }

	void error(string msg, int pos) @safe
	{
		throw new ChainPackReadException(msg, pos);
	}

	long read_int() @safe
	{
		return unpack_int(reader);
	}

	@safe ulong read_uint()
	{
		return unpack_uint(reader);
	}

	@safe RpcValue read_cstring()
	{
		auto app = appender!string();
		get_byte(); // eat '"'
		while(true) {
			auto b = get_byte();
			if(b == '\\') {
				b = get_byte();
				switch (b) {
				case '\\': app.put('\\'); break;
				case '"': app.put('"'); break;
				case 'b': app.put('\b'); break;
				case 'f': app.put('\f'); break;
				case 'n': app.put('\n'); break;
				case 'r': app.put('\r'); break;
				case 't': app.put('\t'); break;
				case '0': app.put('\0'); break;
				default: app.put(cast(dchar) b); break;
				}
			}
			else {
				if (b == '"') {
					// end of string
					break;
				}
				else {
					app.put(cast(dchar) b);
				}
			}
		}
		return RpcValue(app.data);
	}

	RpcValue read_val() @safe
	{
		depth++;
		if(max_depth > 0 && depth > max_depth)
			error("Max depth: " ~ to!string(max_depth) ~ " exceeded.", reader.count);
		RpcValue value;
		RpcValue.Meta meta;
		auto b = peek_byte();
		if(b == PackingSchema.MetaMap) {
			get_byte();
			while (true) {
				b = peek_byte();
				if(b == PackingSchema.TERM) {
					get_byte();
					break;
				}
				RpcValue k = read_val();
				RpcValue v = read_val();
				if(k.type == RpcValue.Type.String) {
					meta[k.str] = v;
				}
				else {
					meta[cast(int) k.integer] = v;
				}
			}
		}

		b = peek_byte();
		if(b < 128) {
			get_byte();
			if((b & 64) != 0) {
				// tiny Int
				value = cast(int) (b & 63);
			}
			else {
				// tiny UInt
				value = cast(uint) (b & 63);
			}
		}
		else {
			switch(b) {
			case PackingSchema.List: {
				get_byte();
				RpcValue.List lst;
				while (true) {
					b = peek_byte();
					if(b == PackingSchema.TERM) {
						get_byte();
						break;
					}
					RpcValue v = read_val();
					lst ~= v;
				}
				value = lst;
				break;
			}
			case PackingSchema.Map: {
				get_byte();
				RpcValue.Map map;
				while (true) {
					b = peek_byte();
					if(b == PackingSchema.TERM) {
						get_byte();
						break;
					}
					RpcValue k = read_val();
					RpcValue v = read_val();
					map[k.str] = v;
				}
				value = map;
				break;
			}
			case PackingSchema.IMap: {
				get_byte();
				RpcValue.IMap map;
				while (true) {
					b = peek_byte();
					if(b == PackingSchema.TERM) {
						get_byte();
						break;
					}
					RpcValue k = read_val();
					RpcValue v = read_val();
					map[cast(int) k.integer] = v;
				}
				value = map;
				break;
			}
			case PackingSchema.Null: {
				get_byte();
				value = null;
				break;
			}
			case PackingSchema.String: {
				get_byte();
				auto len = unpack_uint_data(reader);
				string str;
				for(size_t i=0; i<len; i++) {
					str ~= cast(char) get_byte();
				}
				value = str;
				break;
			}
			case PackingSchema.CString: {
				get_byte();
				string str;
				while(true) {
					b = get_byte();
					if(b == 0)
						break;
					if(b == '\\') {
						b = get_byte();
						switch (b) {
						case '\\': str ~= '\\'; break;
						case '0': str ~= '\0'; break;
						default: str ~= cast(char) b; break;
						}
					}
					else {
						str ~= cast(char) b;
					}
				}
				value = str;
				break;
			}
			case PackingSchema.TRUE: {
				get_byte();
				value = true;
				break;
			}
			case PackingSchema.FALSE: {
				get_byte();
				value = false;
				break;
			}
			case PackingSchema.Int: {
				value = read_int();
				break;
			}
			case PackingSchema.UInt: {
				value = read_uint();
				break;
			}
			case PackingSchema.Decimal: {
				get_byte();
				auto mant = unpack_int_data(reader);
				auto exp = unpack_int_data(reader);
				value = RpcValue.Decimal(mant, cast(int) exp);
				break;
			}
			case PackingSchema.Double: {
				get_byte();
				union U {
					ubyte[double.sizeof] data;
					double d;
				}
				U u;
				for (int i = 0; i < 8; i++)
					u.data[i] = cast(ubyte) get_byte();
				value = u.d;
				break;
			}
			case PackingSchema.DateTime: {
				get_byte();
				long d = unpack_int_data(reader);
				byte offset = 0;
				bool has_tz_offset = (d & 1) != 0;
				bool has_not_msec = (d & 2) != 0;
				d >>= 2;
				if(has_tz_offset) {
					offset = d & 0x7F;
					offset <<= 1;
					offset >>= 1; // sign extension
					d >>= 7;
				}
				if(has_not_msec)
					d *= 1000;
				/*
				debug(chainpack) {
					import std.stdio : writeln;
					writeln("read datetime");
					writeln("has_tz_offset: ", has_tz_offset, "has_not_msec: ", has_not_msec);
					writeln("msecs: ", d);
					writeln("offset: ", offset);
				}
				*/
				d += SHV_EPOCH_MSEC;
				value = RpcValue.DateTime(d, offset * 15);
				break;
			}
			default: {
				throw new Exception("Invalid packing schema: " ~ to!string(b));
				break;
			}
			} // switch
		}
		value.meta = meta;
		return value;
	}

	return read_val();
}

