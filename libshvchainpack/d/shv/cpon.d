module shv.cpon;

import shv.rpcvalue;
import std.range.primitives;
import std.conv;
import std.traits : isSomeChar;
import std.string;

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

struct Writer
{
public:
	this(WriteOptions opts) @safe
	{
		m_options = opts;
	}

	void write(T)(ref T out_range, const ref RpcValue rpcval) @safe
	{
		auto putc = (char c) { out_range.put(c); };
		auto puts = (string s) { foreach(c; s) out_range.put(c); };
		auto meta = rpcval.meta;
		if(meta.length > 0) {
			putc('<');
			if((m_options.isSortKeys) != 0)
				write_map(out_range, meta.bySortedKey(), meta);
			else
				write_map(out_range, meta.byKey(), meta);
			putc('>');
		}
		final switch (rpcval.type)
		{
			case RpcType.Map:
				auto map = rpcval.mapNoRef;
				putc('{');
				if((m_options.isSortKeys) != 0)
					write_map(out_range, sorted_keys!string(map), map);
				else
					write_map(out_range, map.byKey(), map);
				putc('}');
				break;
			case RpcType.IMap:
				auto map = rpcval.imapNoRef;
				puts("i{");
				if((m_options.isSortKeys) != 0)
					write_map(out_range, sorted_keys!int(map), map);
				else
					write_map(out_range, map.byKey(), map);
				putc('}');
				break;

			case RpcType.List:
				auto lst = rpcval.listNoRef;
				putc('[');
				m_nestLevel++;
				auto is_oneliner = Writer.is_oneline_list(lst);
				bool first = true;
				foreach(v; lst) {
					if (!first)
						putc(',');
					indent_item(out_range, is_oneliner, first);
					write(out_range, v);
					first = false;
				}
				m_nestLevel--;
				indent_item(out_range, is_oneliner, true);
				putc(']');
				break;

			case RpcType.String:
				putc('"');
				puts(rpcval.str);
				putc('"');
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

			case RpcType.Null:
				puts("null");
				break;
		}
	}
private:
	enum CponFloatLiteral : string
	{
		Nan = "NaN",       /// string representation of floating-point NaN
		Inf = "Inf",  /// string representation of floating-point Infinity
		NegInf = "-Inf", /// string representation of floating-point negative Infinity
	}

	static bool is_oneline_list(T)(const ref T lst) @safe
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

	static bool is_oneline_map(M)(const ref M map) @safe
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

	void indent_item(T)(ref T out_range, bool is_online_container, bool first_item) @safe
	{
		auto putc = (char c) { out_range.put(c); };
		auto puts = (string s) { foreach(c; s) out_range.put(c); };
		if (m_options.indent.length == 0)
			return;
		if(is_online_container) {
			if(!first_item)
				putc(' ');
		}
		else {
			putc('\n');
            foreach (i; 0 .. m_nestLevel)
				puts(m_options.indent);
		}
	}

	void write_map(T, R, M)(ref T out_range, R keys, const ref M map) @safe
	{
		auto putc = (char c) { out_range.put(c); };
		auto puts = (string s) { foreach(c; s) out_range.put(c); };
		auto is_oneliner = Writer.is_oneline_map(map);
		m_nestLevel++;
		bool first = true;
		foreach(k; keys) {
			if (!first)
				putc(',');
			indent_item(out_range, is_oneliner, first);
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
			write(out_range, rv);
			first = false;
		}
		m_nestLevel--;
		indent_item(out_range, is_oneliner, true);
	}

	private T[] sorted_keys(T)(const ref RpcValue[T] m) @safe {
		T[] ret;
		foreach(k; m.byKey())
			ret ~= k;
		import std.algorithm : sort;
		sort(ret);
		return ret;
	}
private:
	int m_nestLevel = 0;
	WriteOptions m_options;
}


unittest
{
	RpcValue rv = ["foo": "bar"];
	rv["baz"] = 42;
	auto cpon = rv.toCpon();
	import std.stdio : writeln;
	writeln("cpon: ", cpon);
	assert(cpon == `{"baz":42,"foo":"bar"}`);
	rv.meta[1] = 2;
	cpon = rv.toCpon();
	writeln("cpon: ", cpon);
	assert(cpon == `<1:2>{"baz":42,"foo":"bar"}`);
}
/**
Takes a tree of Rpc values and returns the serialized string.

Any Object types will be serialized in a key-sorted order.

If `pretty` is false no whitespaces are generated.
If `pretty` is true serialized string is formatted to be human-readable.
Set the $(LREF RpcOptions.specialFloatLiterals) flag is set in `options` to encode NaN/Infinity as strings.
*/
/+
string toCpon(const ref RpcValue root, in ToCponOptions options = ToCponOptions()) @safe
{
	auto json = appender!string();

	void toStringImpl(Char)(string str) @safe
	{
		json.put('"');

		foreach (Char c; str)
		{
			switch (c)
			{
				case '"':       json.put("\\\"");       break;
				case '\\':      json.put("\\\\");       break;

				case '/':
					if (!(options & RpcOptions.doNotEscapeSlashes))
						json.put('\\');
					json.put('/');
					break;

				case '\b':      json.put("\\b");        break;
				case '\f':      json.put("\\f");        break;
				case '\n':      json.put("\\n");        break;
				case '\r':      json.put("\\r");        break;
				case '\t':      json.put("\\t");        break;
				default:
				{
					import std.ascii : isControl;
					import std.utf : encode;

					// Make sure we do UTF decoding iff we want to
					// escape Unicode characters.
					assert(((options & RpcOptions.escapeNonAsciiChars) != 0)
						== is(Char == dchar), "RpcOptions.escapeNonAsciiChars needs dchar strings");

					with (RpcOptions) if (isControl(c) ||
						((options & escapeNonAsciiChars) >= escapeNonAsciiChars && c >= 0x80))
					{
						// Ensure non-BMP characters are encoded as a pair
						// of UTF-16 surrogate characters, as per RFC 4627.
						wchar[2] wchars; // 1 or 2 UTF-16 code units
						size_t wNum = encode(wchars, c); // number of UTF-16 code units
						foreach (wc; wchars[0 .. wNum])
						{
							json.put("\\u");
							foreach_reverse (i; 0 .. 4)
							{
								char ch = (wc >>> (4 * i)) & 0x0f;
								ch += ch < 10 ? '0' : 'A' - 10;
								json.put(ch);
							}
						}
					}
					else
					{
						json.put(c);
					}
				}
			}
		}

		json.put('"');
	}

	void toString(string str) @safe
	{
		// Avoid UTF decoding when possible, as it is unnecessary when
		// processing Rpc.
		if (options & RpcOptions.escapeNonAsciiChars)
			toStringImpl!dchar(str);
		else
			toStringImpl!char(str);
	}

	void toValue(ref in RpcValue value, ulong indentLevel) @safe
	{
		void putTabs(ulong additionalIndent = 0)
		{
			if (pretty)
				foreach (i; 0 .. indentLevel + additionalIndent)
					json.put("    ");
		}
		void putEOL()
		{
			if (pretty)
				json.put('\n');
		}
		void putCharAndEOL(char ch)
		{
			json.put(ch);
			putEOL();
		}

	}

	toValue(root, 0);
	return json.data;
}
+/
/+ @safe unittest // bugzilla 12897
{
	RpcValue jv0 = RpcValue("test测试");
	assert(toRpc(jv0, false, RpcOptions.escapeNonAsciiChars) == `"test\u6D4B\u8BD5"`);
	RpcValue jv00 = RpcValue("test\u6D4B\u8BD5");
	assert(toRpc(jv00, false, RpcOptions.none) == `"test测试"`);
	assert(toRpc(jv0, false, RpcOptions.none) == `"test测试"`);
	RpcValue jv1 = RpcValue("été");
	assert(toRpc(jv1, false, RpcOptions.escapeNonAsciiChars) == `"\u00E9t\u00E9"`);
	RpcValue jv11 = RpcValue("\u00E9t\u00E9");
	assert(toRpc(jv11, false, RpcOptions.none) == `"été"`);
	assert(toRpc(jv1, false, RpcOptions.none) == `"été"`);
}
+/