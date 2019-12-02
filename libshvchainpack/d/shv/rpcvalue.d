// Written in the D programming language.

/**
ChainPack Object Notation
*/
module shv.rpcvalue;

import std.array;
import std.string;
import std.conv;
import std.range.primitives;
import std.traits;
import std.typecons;
import std.datetime.systime;
import std.datetime.date;
import std.datetime.timezone;
import core.time;

/**
Flags that control how json is encoded and parsed.
*/
enum RpcOptions
{
	none,                       /// standard parsing
	specialFloatLiterals = 0x1, /// encode NaN and Inf float values as strings
	escapeNonAsciiChars = 0x2,  /// encode non ascii characters with an unicode escape sequence
	doNotEscapeSlashes = 0x4,   /// do not escape slashes ('/')
	strictParsing = 0x8,        /// Strictly follow RFC-8259 grammar when parsing
}

unittest {
	Meta m;
	m["foo"] = "bar";
	m[1] = 42;
	m[0] = "baz";

	alias K = Meta.Key;
	assert(m.bySortedKey() == [K(0), K(1), K("foo")]);

	string[] list;
	foreach(k, v; m) {
		auto s = k.toString();
		if(s[0] == '"')
			s = s[1 .. $-1];
		list ~= s ~ ':' ~ to!string(v);
	}
	import std.algorithm : sort;
	sort(list);
	//import std.stdio : writeln;
	//writeln(list);
	assert(list == [`0:"baz"`, `1:42`, `foo:"bar"`]);
}

unittest {
	alias K = Meta.Key;
	Meta m;
	m["a"] = "foo";
	m[1] = 42;

	assert("a" in m);
	assert(("a" in m).str == "foo");
	assert("b" !in m);
	assert(1 in m);
	assert((1 in m).integer == 42);
	assert(2 !in m);
}

struct RpcValue
{
	import std.exception : enforce;

	enum Type : byte
	{
		Invalid,
		Null,
		Bool,
		Integer,
		UInteger,
		Float,
		Decimal,
		DateTime,
		String,
		List,
		Map,
		IMap,
	}

	static struct Meta
	{
		public static struct Key
		{
			private string m_skey;
			private int m_ikey;

			@disable this();
			this(int i) @safe nothrow {m_ikey = i;}
			this(string s) @safe nothrow {m_skey = s;}

			@safe bool isIntKey() const { return m_skey.length == 0; }
			@safe bool isStringKey() const { return m_skey.length > 0; }

			string skey() const @safe pure nothrow {return m_skey; }
			int ikey() const @safe pure nothrow { return m_ikey; }

			size_t toHash() const @safe nothrow
			{
				size_t hash;
				if(m_skey.length > 0) {
					hash = typeid(m_skey).getHash(&m_skey);
				}
				else {
					hash = typeid(m_ikey).getHash(&m_ikey);
				}
				return hash;
			}

			bool opEquals(ref const Key o) const @safe pure nothrow
			{
				return m_ikey == o.m_ikey && m_skey == o.m_skey;
			}
			bool opEquals(string s) const @safe pure nothrow
			{
				return m_skey.length > 0 && m_skey == s;
			}
			bool opEquals(int i) const @safe pure nothrow
			{
				return m_skey.length == 0 && m_ikey == i;
			}
			int opCmp(K)(auto ref K o) const @safe pure nothrow
				if(is(K : Key))
			{
				if(m_skey.length > 0 && o.m_skey.length > 0)
					return std.string.cmp(this.m_skey, o.m_skey);
				if(m_skey.length > 0 && o.m_skey.length == 0)
					return cast(int) m_skey.length;
				if(m_skey.length == 0 && o.m_skey.length > 0)
					return - cast(int) o.m_skey.length;
				return m_ikey - o.m_ikey;
			}
			int opCmp(string s) const @safe nothrow
			{
				return opCmp(Key(s));
			}
			int opCmp(int i) const @safe nothrow
			{
				return opCmp(Key(i));
			}
			unittest {
				assert(Key(1) == 1);
				assert(Key("foo") == "foo");
				assert(Key("foo") > 1);
			}

			void opAssign(T : int)(T arg)
			{
				m_ikey = arg;
				m_skey = null;
			}
			void opAssign(T : string)(T arg)
			{
				m_ikey = 0;
				m_skey = arg;
			}

			unittest {
				import std.algorithm : sort;
				alias K = Key;
				K k1 = 42;
				K k2 = "foo";
				//K[] keys = [1, 2];
				K[] keys = [K(8), K(9), K("Foo"), K("bar"), K(-42), K("Baz")];
				sort(keys);
				K[] sorted = [K(-42), K(8), K(9), K("Baz"), K("Foo"), K("bar")];
				assert(keys == sorted);
			}
			string toString() const pure nothrow @safe
			{
				if(m_skey.length)
					return '"' ~ m_skey ~ '"';
				return to!string(m_ikey);
			}
		}

		auto opBinaryRight(string op : "in")(int k) const @safe
		{
			auto key = Key(k);
			return key in m_values;
		}
		auto opBinaryRight(string op : "in")(string k) const @safe
		{
			auto key = Key(k);
			return key in m_values;
		}
		const(RpcValue) value(K)(K k, RpcValue default_val = RpcValue()) const @safe
		{
			auto p = k in this;
			return (p is null)? default_val: *p;
		}

		ref inout(RpcValue) opIndex(inout Key key) inout @safe { return m_values[key]; }
		ref inout(RpcValue) opIndex(int key) inout @safe { auto k = Key(key); return m_values[k]; }
		ref inout(RpcValue) opIndex(string key) inout @safe { auto k = Key(key); return m_values[k]; }

		void opIndexAssign(T, K)(auto ref T val, K key)
			if(is(K : int) || is(K : string))
		{
			auto k = Key(key);
			m_values[k] = val;
		}

		Key[] bySortedKey() const @safe {
			Key[] ret;
			foreach(k; byKey())
				ret ~= k;
			import std.algorithm : sort;
			sort(ret);
			return ret;
		}
		auto byKey() const @safe { return m_values.byKey();}
		auto byValue() const @safe { return m_values.byValue();}
		auto byKeyValue() const @safe { return m_values.byKeyValue();}
		auto length() const @safe { return m_values.length;}

		int opApply(scope int delegate(Key key, ref RpcValue) dg) @system
		{
			int result;
			foreach (k, v; m_values)
			{
				result = dg(k, v);
				if (result)
					break;
			}
			return result;
		}

		private RpcValue[Key] m_values;
	}


	static struct Decimal
	{
		enum int Base = 10;

		long mantisa;
		int exponent;

		this(long mantisa, int exponent) @safe
		{
			this.mantisa = mantisa;
			this.exponent = exponent;
		}
		this(int dec_places) @safe
		{
			this(0, -dec_places);
		}

		static Decimal fromDouble(double d, int round_to_dec_places) @safe
		{
			int exponent = -round_to_dec_places;
			if(round_to_dec_places > 0) {
				for(; round_to_dec_places > 0; round_to_dec_places--) d *= Base;
			}
			else if(round_to_dec_places < 0) {
				for(; round_to_dec_places < 0; round_to_dec_places++) d /= Base;
			}
			return Decimal(cast(long)(d + 0.5), exponent);
		}

		void setDouble(double d) @safe
		{
			Decimal dc = fromDouble(d, -this.exponent);
			this.mantisa = dc.mantisa;
		}

		double toDouble() const @safe @nogc pure nothrow
		{
			double ret = mantisa;
			int exp = exponent;
			if(exp > 0)
				for(; exp > 0; exp--) ret *= Base;
			else
				for(; exp < 0; exp++) ret /= Base;
			return ret;
		}

		string toString() const @safe
		{
			auto app = appender!string();
			long mant = mantisa;
			if(mant < 0) {
				mant = -mant;
				app.put('-');
			}
			auto str = to!string(mant);
			auto n = str.length;
			auto dec_places = -exponent;
			if(dec_places > 0 && dec_places < n) {
				auto dot_ix = n - dec_places;
				str = str[0 .. dot_ix] ~ '.' ~ str[dot_ix .. $];
			}
			else if(dec_places > 0 && dec_places <= 3) {
				auto extra_0_cnt = dec_places - n;
				string str0 = "0.";
				for (int i = 0; i < extra_0_cnt; ++i)
					str0 ~= '0';
				str = str0 ~ str;
			}
			else if(dec_places < 0 && n + exponent <= 9) {
				for (int i = 0; i < exponent; ++i)
					str ~= '0';
				str ~= '.';
			}
			else if(dec_places == 0) {
				str ~= '.';
			}
			else {
				str ~= 'e' ~ to!string(exponent);
			}
			app.put(str);
			return app.data;
		}
		@safe @nogc int opCmp(ref const Decimal o) const pure nothrow
		{
			if(exponent == o.exponent)
				return cast(int) (mantisa - o.mantisa);
			return cast(int) (toDouble() - o.toDouble());
		}
		@safe @nogc bool opEquals(ref const Decimal o) const pure nothrow
		{
			if(exponent == o.exponent)
				return mantisa == o.mantisa;
			return (toDouble() == o.toDouble());
		}
	}

	static struct DateTime
	{
	private:
		static  struct MsTz
		{
			long hnsec;
			short tz;
		}
		MsTz msecs_tz;
		this(MsTz mz) @safe { msecs_tz = mz; }
		enum long epoch_hnsec = SysTime(std.datetime.date.DateTime(1970, 1, 1, 0, 0, 0), UTC()).stdTime();
	public:
		this(long epoch_msec, int utc_offset_min) @safe
		{
			msecs_tz = MsTz(epoch_msec * 10_000 + epoch_hnsec, cast(short) utc_offset_min);
		}

		long msecsSinceEpoch() const @safe { return (msecs_tz.hnsec - epoch_hnsec) / 10_000; }
		int utcOffsetMin() const @safe { return msecs_tz.tz; }

		static DateTime now() @safe
		{
			SysTime t = Clock.currTime();
			return fromSysTime(t);
		}
		static DateTime fromSysTime(SysTime t) @safe
		{
			MsTz mz;
			mz.hnsec = t.stdTime;
			mz.tz = cast(typeof(MsTz.tz)) t.utcOffset.split!("minutes")().minutes;
			return DateTime(mz);
		}

		static DateTime fromISOExtString(string utc_date_time_str) @safe
		{
			auto ix = utc_date_time_str.lastIndexOfAny("+-");
			if(ix >= 19) {
				string tzs = utc_date_time_str[ix+1 .. $];
				if(tzs.length == 4)
					utc_date_time_str = utc_date_time_str[0 .. ix+1] ~ tzs[0 .. 2] ~ ':' ~ tzs[2 .. $];
				else if(tzs.length == 2)
					utc_date_time_str = utc_date_time_str ~ ":00";
			}
			SysTime t = SysTime.fromISOExtString(utc_date_time_str);
			return fromSysTime(t);
		}
/*
		static DateTime fromISOExtString(string utc_date_time_str) @safe
		{
			enum DT_LEN = 19;
			int ix;
			bool is_neg_offset = false;
			for(ix=DT_LEN; ix<utc_date_time_str.length; ix++) {
				auto c = utc_date_time_str[ix];
				if(c == '+' || c == '-' || c == 'Z') {
					is_neg_offset = (c == '-');
					break;
				}
			}
			static int to_int(char a, char b) { return (a - '0') * 10 + (b - '0'); }
			enforce!Exception(ix < utc_date_time_str.length, "Malformed DateTime string: " ~ utc_date_time_str);
			//auto ix = utc_date_time_str.lastIndexOfAny("+-");
			string tzs = utc_date_time_str[ix+1 .. $];
			int offset_min = 0;
			if(tzs.length >= 2)
				offset_min += 60 * to_int(tzs[0], tzs[1]);
			if(tzs.length == 4)
				offset_min += to_int(tzs[3], tzs[4]);
			string str = utc_date_time_str[0 .. ix] ~ 'Z';
			SysTime t = SysTime.fromISOExtString(str);
			t.stdTime = t.stdTime - offset_min * 60UL * 10_000_000;
			return fromSysTime(t);
		}
*/
		static DateTime fromMSecsSinceEpoch(long msecs, short utc_offset_min = 0) @safe
		{
			MsTz mz;
			mz.hnsec = epoch_hnsec + msecs * 10_000;
			mz.tz = utc_offset_min;
			return DateTime(mz);
		}

		void setMsecsSinceEpoch(long msecs) @safe
		{
			msecs_tz.hnsec = epoch_hnsec + msecs * 10_000;
		}

		void setUtcOffsetMin(short utc_offset_min) @safe {msecs_tz.tz = utc_offset_min;}

		string toLocalString() @safe const
		{
			SysTime t;
			t.stdTime = msecs_tz.hnsec;
			t.timezone = new immutable SimpleTimeZone(minutes(msecs_tz.tz));
			string s = t.toISOExtString();
			auto ix = s.indexOfAny("+-Z");
			if(ix > 0)
				s = s[0 .. ix];
			return s;
		}
		string toISOExtString() @safe const
		{
			static string slice(string str, size_t from, size_t to = size_t.max)
			{
				long len = cast(int) str.length;
				if(len == 0)
					return str;
				if(from >= len)
					from = len - 1;
				if(to > len)
					to = len;
				return str[from .. to];
			}
			enum DT_LEN = 19;
			int msec = (msecs_tz.hnsec / 10_000) % 1000;
			auto t = SysTime(msecs_tz.hnsec + msecs_tz.tz * 60UL * 10_000_000, UTC());
			//t.timezone = UTC();
			//t.timezone = new immutable SimpleTimeZone(minutes(msecs_tz.tz));
			string s1 = t.toISOExtString();
			string s_dt = slice(s1, 0, DT_LEN);
			if(msec > 0) {
				s_dt ~= '.';
				s_dt ~= cast(char) ('0' + (msec / 100));
				msec = msec % 100;
				s_dt ~= cast(char) ('0' + (msec / 10));
				msec = msec % 10;
				s_dt ~= cast(char) ('0' + msec);
			}
			if(msecs_tz.tz == 0) {
				s_dt ~= 'Z';
			}
			else {
				int tz = msecs_tz.tz;
				if(tz < 0) {
					s_dt ~= '-';
					tz = -tz;
				}
				else {
					s_dt ~= '+';
				}
				int h = tz / 60;
				int m = tz % 60;
				s_dt ~= cast(char) ('0' + (h / 10));
				s_dt ~= cast(char) ('0' + (h % 10));
				if(m > 0) {
					s_dt ~= cast(char) ('0' + (m / 10));
					s_dt ~= cast(char) ('0' + (m % 10));
				}
			}
			return s_dt;
		}
		@safe string toString() const {return toISOExtString();}
		@safe int opCmp(ref const DateTime o) const { return cast(int) (msecs_tz.hnsec - o.msecs_tz.hnsec); }
	}
	alias List = RpcValue[];
	alias Map = RpcValue[string];
	alias IMap = RpcValue[int];
	union Store
	{
		string str;
		bool boolean;
		long integer;
		ulong uinteger;
		double floating;
		Decimal decimal;
		DateTime datetime;
		RpcValue[string] map;
		RpcValue[int] imap;
		List list;
	}
	private Store store;
	private Type type_tag = Type.Invalid;
	private Meta m_meta;

	/**
	  Returns the RpcType of the value stored in this structure.
	*/
	Type type() const pure nothrow @safe @nogc
	{
		return type_tag;
	}
	///
	/+ @safe unittest
	{
		  string s = "{ \"language\": \"D\" }";
		  RpcValue j = parseRpc(s);
		  assert(j.type == Type.Map);
		  assert(j["language"].type == Type.String);
	}
	+/

	bool isValid() const @safe { return type() != Type.Invalid; }

	ref inout(Meta) meta() inout @safe { return m_meta; }
	void meta(Meta m) @safe { m_meta = m; }

	/***
	 * Value getter/setter for `Type.String`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `Type.String`.
	 */
	string str() const pure @trusted
	{
		enforce!RpcException(type == Type.String,
								"RpcValue is not a string");
		return store.str;
	}

	string str(string v) pure nothrow @nogc @safe
	{
		assign(v);
		return v;
	}
	///
	@safe unittest
	{
		RpcValue j = [ "language": "D" ];

		// get value
		assert(j["language"].str == "D");

		// change existing key to new string
		j["language"].str = "Perl";
		assert(j["language"].str == "Perl");
	}

	/***
	 * Value getter/setter for `Type.Integer`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `Type.Integer`.
	 */
	long integer() const pure @safe
	{
		enforce!RpcException(type == Type.Integer || type == Type.UInteger,
								"RpcValue is not an integer");
		if(type == Type.UInteger) {
			enforce!RpcException(store.uinteger <= typeof(store.integer).max, "UInt too big to be converted to Int");
			return cast(typeof(store.integer)) store.uinteger;
		}
		return store.integer;
	}

	long integer(long v) pure nothrow @safe @nogc
	{
		assign(v);
		return store.integer;
	}

	/***
	 * Value getter/setter for `Type.UInteger`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `Type.UInteger`.
	 */
	ulong uinteger() const pure @safe
	{
		enforce!RpcException(type == Type.Integer || type == Type.UInteger,
								"RpcValue is not an unsigned integer");
		if(type == Type.Integer) {
			enforce!RpcException(store.integer >= 0, "Negative Int cannot be converted to UInt");
			return cast(typeof(store.uinteger)) store.integer;
		}
		return store.uinteger;
	}

	ulong uinteger(ulong v) pure nothrow @safe @nogc
	{
		assign(v);
		return store.uinteger;
	}

	/***
	 * Value getter/setter for `Type.Float`. Note that despite
	 * the name, this is a $(B 64)-bit `double`, not a 32-bit `float`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `Type.Float`.
	 */
	double floating() const pure @safe
	{
		enforce!RpcException(type == Type.Float,
								"RpcValue is not a floating type");
		return store.floating;
	}

	double floating(double v) pure nothrow @safe @nogc
	{
		assign(v);
		return store.floating;
	}

	bool boolean() const pure @safe
	{
		if (type == Type.Bool) return store.boolean;
		throw new RpcException("RpcValue is not a boolean type");
	}

	bool boolean(bool v) pure nothrow @safe @nogc
	{
		assign(v);
		return store.boolean;
	}
	///
	@safe unittest
	{
		RpcValue j = true;
		assert(j.boolean == true);

		j.boolean = false;
		assert(j.boolean == false);

		j.integer = 12;
		import std.exception : assertThrown;
		assertThrown!RpcException(j.boolean);
	}

	Decimal decimal() const pure @safe
	{
		if (type == Type.Decimal)
			return store.decimal;
		throw new RpcException("RpcValue is not a decimal type");
	}

	Decimal decimal(Decimal v) pure nothrow @safe @nogc
	{
		assign(v);
		return store.decimal;
	}

	DateTime datetime() const pure @safe
	{
		if (type == Type.DateTime)
			return store.datetime;
		throw new RpcException("RpcValue is not a datetime type");
	}

	Decimal decimal(Decimal v) pure nothrow @safe @nogc
	{
		assign(v);
		return store.decimal;
	}

	/***
	 * Value getter/setter for `Type.Map`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `Type.Map`.
	 * Note: this is @system because of the following pattern:
	   ---
	   auto a = &(json.object());
	   json.uinteger = 0;        // overwrite AA pointer
	   (*a)["hello"] = "world";  // segmentation fault
	   ---
	 */
	ref inout(RpcValue[string]) map() inout pure @system
	{
		enforce!RpcException(type == Type.Map,
								"RpcValue is not an object");
		return store.map;
	}

	RpcValue[string] map(RpcValue[string] v) pure nothrow @nogc @safe
	{
		assign(v);
		return v;
	}

	/***
	 * Value getter for `Type.Map`.
	 * Unlike `object`, this retrieves the object by value and can be used in @safe code.
	 *
	 * A caveat is that, if the returned value is null, modifications will not be visible:
	 * ---
	 * RpcValue json;
	 * json.object = null;
	 * json.objectNoRef["hello"] = RpcValue("world");
	 * assert("hello" !in json.object);
	 * ---
	 *
	 * Throws: `RpcException` for read access if `type` is not
	 * `Type.Map`.
	 */
	inout(RpcValue[string]) mapNoRef() inout pure @trusted
	{
		enforce!RpcException(type == Type.Map,
								"RpcValue is not an map");
		return store.map;
	}

	ref inout(RpcValue[int]) imap() inout pure @system
	{
		enforce!RpcException(type == Type.IMap,
								"RpcValue is not an IMap");
		return store.imap;
	}

	RpcValue[int] imap(RpcValue[int] v) pure nothrow @nogc @safe
	{
		assign(v);
		return v;
	}

	inout(RpcValue[int]) imapNoRef() inout pure @trusted
	{
		enforce!RpcException(type == Type.IMap,
								"RpcValue is not an IMap");
		return store.imap;
	}

	/***
	 * Value getter/setter for `Type.List`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `Type.List`.
	 * Note: this is @system because of the following pattern:
	   ---
	   auto a = &(json.array());
	   json.uinteger = 0;  // overwrite array pointer
	   (*a)[0] = "world";  // segmentation fault
	   ---
	 */
	ref inout(RpcValue[]) list() inout pure @system
	{
		enforce!RpcException(type == Type.List,
								"RpcValue is not an list");
		return store.list;
	}

	RpcValue[] list(RpcValue[] v) pure nothrow @nogc @safe
	{
		assign(v);
		return v;
	}

	/***
	 * Value getter for `Type.List`.
	 * Unlike `array`, this retrieves the array by value and can be used in @safe code.
	 *
	 * A caveat is that, if you append to the returned array, the new values aren't visible in the
	 * RpcValue:
	 * ---
	 * RpcValue json;
	 * json.array = [RpcValue("hello")];
	 * json.arrayNoRef ~= RpcValue("world");
	 * assert(json.array.length == 1);
	 * ---
	 *
	 * Throws: `RpcException` for read access if `type` is not
	 * `Type.List`.
	 */
	inout(RpcValue[]) listNoRef() inout pure @trusted
	{
		enforce!RpcException(type == Type.List,
								"RpcValue is not an list");
		return store.list;
	}

	/// Test whether the type is `Type.null_`
	bool isNull() const pure nothrow @safe @nogc
	{
		return type == Type.Null;
	}

	private void assign(T)(T arg) @safe
	{
		static if (is(T : typeof(null)))
		{
			type_tag = Type.Null;
		}
		else static if (is(T : bool))
		{
			type_tag = Type.Bool;
			bool t = arg;
			() @trusted { store.boolean = t; }();
		}
		else static if (is(T : string))
		{
			type_tag = Type.String;
			string t = arg;
			() @trusted { store.str = t; }();
		}
		else static if (is(T : Decimal))
		{
			type_tag = Type.Decimal;
			Decimal t = arg;
			() @trusted { store.decimal = t; }();
		}
		else static if (isSomeString!T) // issue 15884
		{
			type_tag = Type.String;
			// FIXME: std.Array.Array(Range) is not deduced as 'pure'
			() @trusted {
				import std.utf : byUTF;
				store.str = cast(immutable)(arg.byUTF!char.array);
			}();
		}
		else static if (is(T : bool))
		{
			type_tag = Type.Bool;
			store.boolean = arg;
		}
		else static if (is(T : ulong) && isUnsigned!T)
		{
			type_tag = Type.UInteger;
			store.uinteger = arg;
		}
		else static if (is(T : long))
		{
			type_tag = Type.Integer;
			store.integer = arg;
		}
		else static if (isFloatingPoint!T)
		{
			type_tag = Type.Float;
			store.floating = arg;
		}
		else static if (is(T : Decimal)) {
			type_tag = Type.Decimal;
			store.datetime = arg;
		}
		else static if (is(T : DateTime)) {
			type_tag = Type.DateTime;
			store.datetime = arg;
		}
		else static if (is(T : SysTime)) {
			type_tag = Type.DateTime;
			store.datetime = DateTime.fromSysTime(arg);
		}
		else static if (is(T : Value[Key], Key, Value))
		{
			static if(is(Key : string)) {
				type_tag = Type.Map;
				static if (is(Value : RpcValue))
				{
					RpcValue[Key] t = arg;
					() @trusted { store.map = t; }();
				}
				else
				{
					RpcValue[string] aa;
					foreach (key, value; arg)
						aa[key] = RpcValue(value);
					() @trusted { store.map = aa; }();
				}
			}
			else static if(is(Key : int)) {
				type_tag = Type.IMap;
				static if (is(Value : RpcValue))
				{
					RpcValue[Key] t = arg;
					() @trusted { store.imap = t; }();
				}
				else
				{
					RpcValue[string] aa;
					foreach (key, value; arg)
						aa[key] = RpcValue(value);
					() @trusted { store.imap = aa; }();
				}
			}
			else {
				static assert(false, "Unknown AA key type");
			}
		}
		else static if (isArray!T)
		{
			type_tag = Type.List;
			static if (is(ElementEncodingType!T : RpcValue))
			{
				RpcValue[] t = arg;
				() @trusted { store.list = t; }();
			}
			else
			{
				RpcValue[] new_arg = new RpcValue[arg.length];
				foreach (i, e; arg)
					new_arg[i] = RpcValue(e);
				() @trusted { store.list = new_arg; }();
			}
		}
		else static if (is(T : RpcValue))
		{
			type_tag = arg.type;
			store = arg.store;
		}
		else
		{
			static assert(false, text(`unable to convert type "`, T.Stringof, `" to json`));
		}
	}

	private void assignRef(T)(ref T arg) if (isStaticArray!T)
	{
		type_tag = Type.List;
		static if (is(ElementEncodingType!T : RpcValue))
		{
			store.list = arg;
		}
		else
		{
			RpcValue[] new_arg = new RpcValue[arg.length];
			foreach (i, e; arg)
				new_arg[i] = RpcValue(e);
			store.list = new_arg;
		}
	}

	/**
	 * Constructor for `RpcValue`. If `arg` is a `RpcValue`
	 * its value and type will be copied to the new `RpcValue`.
	 * Note that this is a shallow copy: if type is `Type.Map`
	 * or `Type.List` then only the reference to the data will
	 * be copied.
	 * Otherwise, `arg` must be implicitly convertible to one of the
	 * following types: `typeof(null)`, `string`, `ulong`,
	 * `long`, `double`, an associative array `V[K]` for any `V`
	 * and `K` i.e. a Rpc object, any array or `bool`. The type will
	 * be set accordingly.
	 */
	this(T)(T arg) if (!isStaticArray!T)
	{
		assign(arg);
	}
	/// Ditto
	this(T)(ref T arg) if (isStaticArray!T)
	{
		assignRef(arg);
	}
	/// Ditto
	this(T : RpcValue)(inout T arg) inout
	{
		store = arg.store;
		type_tag = arg.type;
	}
	///
	@safe unittest
	{
		RpcValue j = RpcValue( "a string" );
		j = RpcValue(42);

		j = RpcValue( [1, 2, 3] );
		assert(j.type == Type.List);

		j = RpcValue( ["language": "D"] );
		assert(j.type == Type.Map);
	}

	void opAssign(T)(T arg) if (!isStaticArray!T && !is(T : RpcValue))
	{
		assign(arg);
	}

	void opAssign(T)(ref T arg) if (isStaticArray!T)
	{
		assignRef(arg);
	}

	/***
	 * Array syntax for json arrays.
	 * Throws: `RpcException` if `type` is not `Type.List`.
	 */
	ref inout(RpcValue) opIndex(size_t i) inout pure @safe
	{
		auto a = this.listNoRef;
		enforce!RpcException(i < a.length,
								"RpcValue array index is out of range");
		return a[i];
	}
	///
	@safe unittest
	{
		RpcValue j = RpcValue( [42, 43, 44] );
		assert( j[0].integer == 42 );
		assert( j[1].integer == 43 );
	}

	/***
	 * Hash syntax for json objects.
	 * Throws: `RpcException` if `type` is not `Type.Map`.
	 */
	ref inout(RpcValue) opIndex(string k) inout pure @safe
	{
		auto o = this.mapNoRef;
		return *enforce!RpcException(k in o,
										"Key not found: " ~ k);
	}

	/***
	 * Operator sets `value` for element of Rpc object by `key`.
	 *
	 * If Rpc value is null, then operator initializes it with object and then
	 * sets `value` for it.
	 *
	 * Throws: `RpcException` if `type` is not `Type.Map`
	 * or `Type.null_`.
	 */
	void opIndexAssign(T)(auto ref T value, string key) pure
	{
		enforce!RpcException(type == Type.Map || type == Type.Null,
								"RpcValue must be object or null");
		RpcValue[string] aa = null;
		if (type == Type.Map) {
			aa = this.mapNoRef;
		}

		aa[key] = value;
		this.map = aa;
	}

	void opIndexAssign(T)(T arg, size_t i) pure
	{
		auto a = this.arrayNoRef;
		enforce!RpcException(i < a.length, "RpcValue array index is out of range");
		a[i] = arg;
		this.array = a;
	}
	///
	@safe unittest
	{
			RpcValue j = RpcValue( ["Perl", "C"] );
			j[1].str = "D";
			assert( j[1].str == "D" );
	}

	RpcValue opBinary(string op : "~", T)(T arg) @safe
	{
		auto a = this.arrayNoRef;
		static if (isArray!T)
		{
			return RpcValue(a ~ RpcValue(arg).arrayNoRef);
		}
		else static if (is(T : RpcValue))
		{
			return RpcValue(a ~ arg.arrayNoRef);
		}
		else
		{
			static assert(false, "argument is not an array or a RpcValue array");
		}
	}

	void opOpAssign(string op : "~", T)(T arg) @safe
	{
		auto a = this.listNoRef;
		static if (isArray!T)
		{
			a ~= RpcValue(arg).listNoRef;
		}
		else static if (is(T : RpcValue))
		{
			a ~= arg.listNoRef;
		}
		else
		{
			static assert(false, "argument is not an array or a RpcValue array");
		}
		this.list = a;
	}

	/**
	 * Support for the `in` operator.
	 *
	 * Tests wether a key can be found in an object.
	 *
	 * Returns:
	 *      when found, the `const(RpcValue)*` that matches to the key,
	 *      otherwise `null`.
	 *
	 * Throws: `RpcException` if the right hand side argument `RpcType`
	 * is not `object`.
	 */
	auto opBinaryRight(string op : "in")(string k) const @safe
	{
		return k in this.mapNoRef;
	}
	///
	@safe unittest
	{
		RpcValue j = [ "language": "D", "author": "walter" ];
		string a = ("author" in j).str;
	}

	bool opEquals(const RpcValue rhs) const @nogc nothrow pure @safe
	{
		return opEquals(rhs);
	}

	bool opEquals(ref const RpcValue rhs) const @nogc nothrow pure @trusted
	{
		// Default doesn't work well since store is a union.  Compare only
		// what should be in store.
		// This is @trusted to remain nogc, nothrow, fast, and usable from @safe code.

		final switch (type_tag)
		{
		case Type.Invalid:
			switch (rhs.type_tag)
			{
				case Type.Invalid:
					return true;
				default:
					return false;
			}
		case Type.Integer:
			switch (rhs.type_tag)
			{
				case Type.Integer:
					return store.integer == rhs.store.integer;
				case Type.UInteger:
					return store.integer == rhs.store.uinteger;
				case Type.Float:
					return store.integer == rhs.store.floating;
				default:
					return false;
			}
		case Type.UInteger:
			switch (rhs.type_tag)
			{
				case Type.Integer:
					return store.uinteger == rhs.store.integer;
				case Type.UInteger:
					return store.uinteger == rhs.store.uinteger;
				case Type.Float:
					return store.uinteger == rhs.store.floating;
				default:
					return false;
			}
		case Type.Float:
			switch (rhs.type_tag)
			{
				case Type.Integer:
					return store.floating == rhs.store.integer;
				case Type.UInteger:
					return store.floating == rhs.store.uinteger;
				case Type.Float:
					return store.floating == rhs.store.floating;
				default:
					return false;
			}
		case Type.Bool:
			return type_tag == rhs.type_tag && store.boolean == rhs.store.boolean;
		case Type.Decimal:
			return type_tag == rhs.type_tag && store.decimal == rhs.store.decimal;
		case Type.DateTime:
			return type_tag == rhs.type_tag && store.datetime == rhs.store.datetime;
		case Type.String:
			return type_tag == rhs.type_tag && store.str == rhs.store.str;
		case Type.Map:
			return type_tag == rhs.type_tag && store.map == rhs.store.map;
		case Type.IMap:
			return type_tag == rhs.type_tag && store.imap == rhs.store.imap;
		case Type.List:
			return type_tag == rhs.type_tag && store.list == rhs.store.list;
		case Type.Null:
			return type_tag == rhs.type_tag;
		}
	}

	///
	@safe unittest
	{
		assert(RpcValue(0u) == RpcValue(0));
		assert(RpcValue(0u) == RpcValue(0.0));
		assert(RpcValue(0) == RpcValue(0.0));
	}

	/// Implements the foreach `opApply` interface for json arrays.
	int opApply(scope int delegate(size_t index, ref RpcValue) dg) @system
	{
		int result;

		foreach (size_t index, ref value; list)
		{
			result = dg(index, value);
			if (result)
				break;
		}

		return result;
	}

	/// Implements the foreach `opApply` interface for json objects.
	int opApply(scope int delegate(string key, ref RpcValue) dg) @system
	{
		enforce!RpcException(type == Type.Map,
								"RpcValue is not an object");
		int result;

		foreach (string key, ref value; map)
		{
			result = dg(key, value);
			if (result)
				break;
		}

		return result;
	}

	/*
	string toCpon(Flag!"pretty" pretty = No.pretty) const
	{
		import shv.cpon : write, WriteOptions;
		auto app = appender!string();
		WriteOptions opts;
		opts.sortKeys = true;
		opts.indent = pretty? "\t": "";
		write(app, this, opts);
		//import std.stdio : writeln;
		//writeln('*', a.data);
		return app.data;
	}

	//string toString() const @safe { return toCpon(No.pretty); }
	static RpcValue fromCpon(string cpon)
	{
		import shv.cpon : read;
		return read(cpon);
	}
	ubyte[] toChainPack() const
	{
		import shv.chainpack : write;
		auto app = appender!(ubyte[])();
		write(app, this);
		return app.data;
	}
	static RpcValue fromChainPack(R)(ref R input_range)
	{
		import shv.chainpack : read;
		return read!R(input_range);
	}
	*/
}

/+ @safe unittest
{
	enum issue15742objectOfObject = `{ "key1": { "key2": 1 }}`;
	static assert(parseRpc(issue15742objectOfObject).type == Type.Map);

	enum issue15742arrayOfArray = `[[1]]`;
	static assert(parseRpc(issue15742arrayOfArray).type == Type.List);
}
+/

/+ @safe unittest
{
	// Ensure we can read and use Rpc from @safe code
	auto a = `{ "key1": { "key2": 1 }}`.parseRpc;
	assert(a["key1"]["key2"].integer == 1);
	assert(a.toString == `{"key1":{"key2":1}}`);
}
+/

/+ @system unittest
{
	// Ensure we can read Rpc from a @system range.
	struct Range
	{
		string s;
		size_t index;
		@system
		{
			bool empty() { return index >= s.length; }
			void popFront() { index++; }
			char front() { return s[index]; }
		}
	}
	auto s = Range(`{ "key1": { "key2": 1 }}`);
	auto json = parseRpc(s);
	assert(json["key1"]["key2"].integer == 1);
}
+/

/**
Exception thrown on Rpc errors
*/
class RpcException : Exception
{
	this(string msg, int line = 0, int pos = 0) pure nothrow @safe
	{
		if (line)
			super(text(msg, " (Line ", line, ":", pos, ")"));
		else
			super(msg);
	}

	this(string msg, string file, size_t line) pure nothrow @safe
	{
		super(msg, file, line);
	}
}


