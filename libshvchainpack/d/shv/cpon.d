module shv.cpon;

import shv.rpcvalue;
import std.array;
import std.range.primitives;
import std.conv;
import std.traits;// : isSomeChar;
import std.string;
import std.utf;
import std.exception;
debug {
	import std.stdio : writeln;
}
/*
struct WriteOptions
{
private:
	bool m_sortKeys = false;
	string m_indent;
public:
	ref WriteOptions setSortKeys(bool on) nothrow @safe { m_sortKeys = on; return this; }
	bool isSortKeys() const nothrow @safe { return m_sortKeys; }

	ref WriteOptions setIndent(string s) nothrow @safe { m_indent = s; return this; }
	string indent() const nothrow @safe { return m_indent; }
}
*/
struct WriteOptions
{
	bool sortKeys = false;
	string indent;
	bool writeInvalidAsNull = true;
}

enum CponFloatLiteral : string
{
	Nan = "NaN",       /// string representation of floating-point NaN
	Inf = "Inf",  /// string representation of floating-point Infinity
	NegInf = "-Inf", /// string representation of floating-point negative Infinity
}

void write(T)(ref T out_range, const ref RpcValue rpcval, WriteOptions opts = WriteOptions()) @safe
{
	int m_nestLevel = 0;

	void putc(char c) { out_range.put(c); }
	void puts(string s) { foreach(c; s) out_range.put(c); }

	bool is_oneline_list(T)(const ref T lst) @safe
	{
		if (lst.length > 10)
			return false;
		foreach(v; lst) {
			if(v.type == RpcType.Map
				|| v.type == RpcType.IMap
				|| v.type == RpcType.List)
				return false;
		}
		return true;
	}

	bool is_oneline_map(M)(const ref M map) @safe
	{
		if (map.length > 10)
			return false;
		foreach(v; map.byValue()) {
			if(v.type == RpcType.Map
				|| v.type == RpcType.IMap
				|| v.type == RpcType.List)
				return false;
		}
		return true;
	}

	void indent_item(bool is_online_container, bool first_item) @safe
	{
		if (opts.indent.length == 0)
			return;
		if(is_online_container) {
			if(!first_item)
				putc(' ');
		}
		else {
			putc('\n');
			foreach (i; 0 .. m_nestLevel)
				puts(opts.indent);
		}
	}

	void write_map(R, M)(R keys, const ref M map) @safe
	{
		auto is_oneliner = is_oneline_map(map);
		m_nestLevel++;
		bool first = true;
		foreach(k; keys) {
			if (!first)
				putc(',');
			indent_item(is_oneliner, first);
			static if (is(string : typeof(k))) {
				putc('"');
				puts(k);
				putc('"');
			}
			else {
				puts(to!string(k));
			}
			putc(':');
			auto rv = map[k];
			write_rpcval(rv);
			first = false;
		}
		m_nestLevel--;
		indent_item(is_oneliner, true);
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
		putc('"');
		while(!str.empty) {
			auto c = str.decodeFront();
			debug(cstring) {
				writeln(">", c, " ", c == '\t');
			}
			switch (c)
			{
				case '"':       puts("\\\"");       break;
				case '\\':      puts("\\\\");       break;
				case '\b':      puts("\\b");        break;
				case '\f':      puts("\\f");        break;
				case '\n':      puts("\\n");        break;
				case '\r':      puts("\\r");        break;
				case '\t':      puts("\\t");        break;
				default:
				{
					import std.ascii : isControl;
					import std.utf : encode;

					if (c >= 0x80) {
						char[4] chars;
						size_t n = encode(chars, c); // number of UTF-8 code units
						foreach (wc; chars[0 .. n])
							putc(wc);
					}
					else {
						putc(c % 256);
					}
				}
			}
		}

		putc('"');
	}

	@safe
	void write_rpcval(const ref RpcValue rpcval)
	{
		auto meta = rpcval.meta;
		if(meta.length > 0) {
			putc('<');
			if((opts.sortKeys) != 0)
				write_map(meta.bySortedKey(), meta);
			else
				write_map(meta.byKey(), meta);
			putc('>');
		}
		final switch (rpcval.type)
		{
			case RpcType.Map:
				auto map = rpcval.mapNoRef;
				putc('{');
				if((opts.sortKeys) != 0)
					write_map(sorted_keys!string(map), map);
				else
					write_map(map.byKey(), map);
				putc('}');
				break;
			case RpcType.IMap:
				auto map = rpcval.imapNoRef;
				puts("i{");
				if((opts.sortKeys) != 0)
					write_map(sorted_keys!int(map), map);
				else
					write_map(map.byKey(), map);
				putc('}');
				break;

			case RpcType.List:
				auto lst = rpcval.listNoRef;
				putc('[');
				m_nestLevel++;
				auto is_oneliner = is_oneline_list(lst);
				bool first = true;
				foreach(v; lst) {
					if (!first)
						putc(',');
					indent_item(is_oneliner, first);
					write_rpcval(v);
					first = false;
				}
				m_nestLevel--;
				indent_item(is_oneliner, true);
				putc(']');
				break;

			case RpcType.String:
				write_cstring(rpcval.str);
				break;

			case RpcType.Integer:
				puts(to!string(rpcval.integer));
				break;

			case RpcType.UInteger:
				puts(to!string(rpcval.uinteger));
				putc('u');
				break;

			case RpcType.Float:
				import std.math : isNaN, isInfinity;

				auto val = rpcval.floating;
				if (val.isNaN)
				{
					puts(CponFloatLiteral.Nan);
				}
				else if (val.isInfinity)
				{
					puts((val > 0) ?  CponFloatLiteral.Inf : CponFloatLiteral.NegInf);
				}
				else
				{
					import std.format : format;
					// The correct formula for the number of decimal digits needed for lossless round
					// trips is actually:
					//     ceil(log(pow(2.0, double.mant_dig - 1)) / log(10.0) + 1) == (double.dig + 2)
					// Anything less will round off (1 + double.epsilon)
					puts("%.18g".format(val));
				}
				break;

			case RpcType.Bool:
				puts(rpcval.boolean? "true": "false");
				break;
			case RpcType.Decimal:
				puts(rpcval.decimal.toString());
				break;
			case RpcType.DateTime:
				puts(rpcval.datetime.toISOExtString());
				break;
			case RpcType.Null:
				puts("null");
				break;
			case RpcType.Invalid:
				enforce(opts.writeInvalidAsNull, "Cannot write Invalid RpcValue.");
				break;
		}
	}

	write_rpcval(rpcval);
}

unittest
{
	RpcValue rv = ["foo": "bar"];
	rv["baz"] = 42;
	auto cpon = rv.toCpon();
	assert(cpon == `{"baz":42,"foo":"bar"}`);
	rv.meta[1] = 2;
	cpon = rv.toCpon();
	assert(cpon == `<1:2>{"baz":42,"foo":"bar"}`);
}

@safe unittest
{
	RpcValue jv0 = RpcValue("\test测试");
	debug(cstring) {
		writeln(jv0.str(), " cpon: ", jv0.toCpon());
	}
	assert(jv0.toCpon() == `"\test测试"`);
}

class CponParseException : Exception
{
	this(string msg, int line = 0, int pos = 0) pure nothrow @safe
	{
		if (line)
			super(text(msg, " (Line ", line, ":", pos, ")"));
		else
			super(msg);
	}
}

RpcValue parse(int max_depth = -1)(string cpon) @safe
{
	auto a = cast(immutable (ubyte)[]) cpon;
	return parse!(immutable (ubyte)[], max_depth)(a);
}

RpcValue parse(T, int max_depth = -1)(T cpon) @safe
if (isInputRange!T && !isInfinite!T && is(Unqual!(ElementType!T) == ubyte))
{
	//import std.ascii : isDigit, isHexDigit, toUpper, toLower;
	//import std.typecons : Nullable, Yes;
	//debug writeln("neco: ", typeid(Unqual!(ElementType!T)));

	int depth = -1;
	//Nullable!Char next;
	int line = 1, pos = 0;
	enum NO_CHAR = -1;
	alias Char = short; //Unqual!(ElementType!T);

	Char peek_char() @safe
	{
		if(cpon.empty())
			return NO_CHAR;
		return cpon.front();
	}

	Char get_char() @safe
	{
		Char ret = cpon.front();
		cpon.popFront();
		if(ret == '\n') {
			line++;
			pos = 0;
		}
		else {
			pos++;
		}
		return ret;
	}

	void error(string msg) @safe
	{
		string near;
		for(int i=0; i<100; i++) {
			auto c = peek_char();
			if(c < 0)
				break;
			get_char();
			near ~= cast(char) c;
		}
		throw new CponParseException(msg ~ " near: " ~ near, line, pos);
	}

	void get_token(string s) @safe
	{
		foreach(c; s) {
			if(c != get_char())
				error("Token '" ~ s ~ "' expected.");
		}
	}

	void skip_white_insignificant() @safe
	{
		while (true) {
			auto b = peek_char();
			if (b < 0)
				return;
			if (b > ' ') {
				if (b == '/') {
					get_char();
					b = get_char();
					if (b == '*') {
						// multiline_comment_entered
						while (true) {
							b = get_char();
							if (b == '*') {
								b = get_char();
								if (b == '/')
									break;
							}
						}
					}
					else if (b == '/') {
						// to end of line comment entered
						while (true) {
							b = get_char();
							if (b == '\n')
								break;
						}
					}
					else
						error("Malformed comment");
				}
				else if (b == ':') {
					get_char();
					continue;
				}
				else if (b == ',') {
					get_char();
					continue;
				}
				else {
					break;
				}
			}
			else {
				get_char();
			}
		}
	}

	long read_int(out int digit_cnt) @safe
	{
		auto base = 10;
		long val = 0;
		bool neg = false;
		int n = 0;
		while (true) {
			auto b = peek_char();
			if(b < 0)
				break;
			if (b == '+' || b == '-') { // '+','-'
				if(n != 0)
					break;
				get_char();
				if(b == '-')
					neg = true;
			}
			else if (b == 'x') { // 'x'
				if(n == 1 && val != 0)
					break;
				if(n != 1)
					break;
				get_char();
				base = 16;
			}
			else if( b >= '0' && b <= '9') { // '0' - '9'
				get_char();
				val *= base;
				val += b - 48;
				digit_cnt++;
			}
			else if( b >= 'A' && b <= 'F') { // 'A' - 'F'
				if(base != 16)
					break;
				get_char();
				val *= base;
				val += b - 65 + 10;
				digit_cnt++;
			}
			else if( b >= 'a' && b <= 'f') { // 'a' - 'f'
				if(base != 16)
					break;
				get_char();
				val *= base;
				val += b - 97 + 10;
				digit_cnt++;
			}
			else {
				break;
			}
			n++;
		}

		if(neg)
			val = -val;
		return val;

	}

	@safe RpcValue read_number()
	{
		long mantisa = 0;
		int exponent = 0;
		int decimals = 0;
		int dec_cnt = 0;
		bool is_decimal = false;
		bool is_uint = false;
		bool is_neg = false;
		int digit_cnt;

		auto b = peek_char();
		if(b == '+') {// '+'
			is_neg = false;
			get_char();
		}
		else if(b == '-') {// '-'
			is_neg = true;
			get_char();
		}

		mantisa = read_int(digit_cnt);
		b = peek_char();
		while(b > 0) {
			if(b == 'u') {
				is_uint = true;
				get_char();
				break;
			}
			if(b == '.') {
				is_decimal = true;
				get_char();
				decimals = cast(int) read_int(dec_cnt);
				b = peek_char();
				if(b == NO_CHAR)
					break;
			}
			if(b == 'e' || b == 'E') {
				is_decimal = true;
				get_char();
				exponent = cast(int) read_int(digit_cnt);
				if(digit_cnt == 0)
					error("Malformed number exponetional part.");
				break;
			}
			break;
		}
		if(is_decimal) {
			for (int i = 0; i < dec_cnt; ++i)
				mantisa *= 10;
			mantisa += decimals;
			mantisa = is_neg? -mantisa: mantisa;
			return RpcValue(RpcValue.Decimal(mantisa, exponent - dec_cnt));
		}
		else if(is_uint) {
			return RpcValue(cast(ulong) mantisa);
		}
		else {
			return RpcValue(mantisa);
		}
	}

	RpcValue read_datetime()
	{
		get_char(); // eat '"'
		string s;
		while (true) {
			auto c = get_char();
			if(c == '"')
				break;
		}
		auto dt = RpcValue.DateTime.fromISOExtString(s);
		return RpcValue(dt);
	}

	@safe RpcValue read_cstring()
	{
		auto app = appender!string();
		get_char(); // eat '"'
		while(true) {
			auto b = get_char();
			if(b == '\\') {
				b = get_char();
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
		if(depth > 0 && depth > max_depth)
			error("Max depth: " ~ to!string(max_depth) ~ " exceeded.");
		skip_white_insignificant();
		RpcValue value;
		Meta meta;
		auto b = peek_char();

		if (b == '<') {
			b = get_char();  // eat '<'
			while (true) {
				skip_white_insignificant();
				b = peek_char();
				if (b == '>') {
					get_char();
					break;
				}
				auto key = read_val();
				skip_white_insignificant();
				auto val = read_val();
				if (key.type == RpcType.String)
					meta[key.str] = val;
				else if (key.type == RpcType.Integer)
					meta[ cast(int) key.integer] = val;
				else if (key.type == RpcType.UInteger)
					meta[ cast(int) key.uinteger] = val;
				else
					error("Malformed meta, invalid key: " ~ to!string(key));
			}
		}

		skip_white_insignificant();
		b = peek_char();
		debug {
			writeln("c: ", cast(char) b);
		}
		switch(b) {
		case '0': .. case '9':
		case '+':
		case '-':
			value = read_number();
			break;
		case '"':
			value = read_cstring();
			break;
		case '[': {
			get_char();  // eat '['
			RpcValue[] lst;
			while (true) {
				skip_white_insignificant();
				b = peek_char();
				if (b == ']') {
					get_char();
					break;
				}
				auto val = read_val();
			}
			value = lst;
			break;
		}
		case '{': {
			get_char();  // eat '{'
			RpcValue[string] mmap;
			while (true) {
				skip_white_insignificant();
				b = peek_char();
				if (b == '}') {
					get_char();
					break;
				}
				auto key = read_val();
				skip_white_insignificant();
				auto val = read_val();
				if (key.type == RpcType.String)
					mmap[key.str] = val;
				else
					error("Malformed map, invalid key: " ~ to!string(key));
			}
			value = mmap;
			break;
		}
		case 'i': {
			get_char();
			b = peek_char();
			if (b == '{') {
				get_char();  // eat '{'
				RpcValue[int] mmap;
				while (true) {
					skip_white_insignificant();
					b = peek_char();
					if (b == '}') {
						get_char();
						break;
					}
					auto key = read_val();
					skip_white_insignificant();
					auto val = read_val();
					if (key.type == RpcType.Integer)
						mmap[cast(int) key.integer] = val;
					else if (key.type == RpcType.UInteger)
						mmap[cast(int) key.uinteger] = val;
					else
						error("Malformed imap, invalid key: " ~ to!string(key));
				}
				value = mmap;
			}
			else
				error("Invalid IMap prefix.");
			break;
		}
		case 'd':
			get_char();
			b = peek_char();
			if (b == '"')
				value = read_datetime();
			else
				error("Invalid DateTime prefix.");
			break;
		case 't':
			get_token("true");
			value = true;
			break;
		case 'f':
			get_token("false");
			value = false;
			break;
		case 'n':
			get_token("null");
			value = null;
			break;
		default:
			error("Malformed Cpon input.");
		}
		value.meta = meta;
		depth--;
		return value;
	}
	return read_val();
}

@system unittest
{
	string s1 = `"abc"`;
	RpcValue rv = parse(s1);
	assert(rv.type == RpcType.String);
	assert(('"' ~ rv.str ~ '"') == s1);
}