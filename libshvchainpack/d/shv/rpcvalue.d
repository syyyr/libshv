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

/**
Rpc type enumeration
*/
enum RpcType : byte
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

struct Meta
{
	public static struct Key
	{
		private string m_skey;
		private int m_ikey;

		@disable this();
		this(int i) @safe nothrow {m_ikey = i;}
		this(string s) @safe nothrow {m_skey = s;}

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

	//this(ref return scope const Meta o)
	//{
	//	m_values = o.m_values;
	//}

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

	struct Decimal
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

		double toDouble() const @safe
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
	}

	struct DateTime
	{
	private:
		struct MsTz
		{
			//import std.bitmanip : bitfields;
			//mixin(bitfields!(
			//	int, "tz", 7,
			//	long,  "msec", 57));
			long hnsec;
			short tz;
		}
		MsTz msecs_tz;
		this(MsTz mz) @safe { msecs_tz = mz; }
		enum long epoch_hnsec = SysTime(std.datetime.date.DateTime(1970, 1, 1, 0, 0, 0), UTC()).stdTime();
	public:
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
		//static DateTime fromLocalString(string local_date_time_str) @safe
		//{
		//	SysTime t = SysTime.fromISOExtString(local_date_time_str);
		//	return fromSysTime(t);
		//}
		static DateTime fromISOExtString(string utc_date_time_str) @safe
		{
			auto ix = utc_date_time_str.lastIndexOfAny("+-");
			if(ix >= 19) {
				string tzs = utc_date_time_str[ix+1 .. $];
				if(tzs.length == 4)
					utc_date_time_str = utc_date_time_str[0 .. ix+1] ~ tzs[0 .. 2] ~ ':' ~ tzs[2 .. $];
			}
			SysTime t = SysTime.fromISOExtString(utc_date_time_str);
			return fromSysTime(t);
		}

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
			SysTime t;
			t.stdTime = msecs_tz.hnsec;
			t.timezone = new immutable SimpleTimeZone(minutes(msecs_tz.tz));
			if(msecs_tz.tz == 0) {
				string s = t.toISOExtString();
				s = s[0 .. $-6] ~ 'Z';
				return s;
			}
			else {
				string s = t.toISOExtString();
				auto ix = s.lastIndexOfAny("+-");
				if(ix >= 19) {
					string tz = s[ix+1 .. $];
					if(tz.length == 5) {
						string tz2 = tz[0 .. 2];
						if(tz[3 .. 5] != "00")
							tz2 ~= tz[3 .. 5];
						s = s[0 .. ix+1] ~ tz2;
					}
				}
				return s;
			}
		}
		@safe string toString() const {return toISOExtString();}
		//@safe bool operator ==(const DateTime &o) const { return (msecs_tz.msec == o.msecs_tz.msec); }
		@safe int opCmp(ref const DateTime o) const { return cast(int) (msecs_tz.hnsec - o.msecs_tz.hnsec); }
	}

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
		RpcValue[] list;
	}
	private Store store;
	private RpcType type_tag;
	public Meta m_meta;

	/**
	  Returns the RpcType of the value stored in this structure.
	*/
	RpcType type() const pure nothrow @safe @nogc
	{
		return type_tag;
	}
	///
	/+ @safe unittest
	{
		  string s = "{ \"language\": \"D\" }";
		  RpcValue j = parseRpc(s);
		  assert(j.type == RpcType.Map);
		  assert(j["language"].type == RpcType.String);
	}
	+/

	ref inout(Meta) meta() inout @safe { return m_meta; }
	//const(Meta) meta() const @safe { return m_meta; }
	void meta(Meta m) @safe { m_meta = m; }

	/***
	 * Value getter/setter for `RpcType.String`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `RpcType.String`.
	 */
	string str() const pure @trusted
	{
		enforce!RpcException(type == RpcType.String,
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
	 * Value getter/setter for `RpcType.Integer`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `RpcType.Integer`.
	 */
	long integer() const pure @safe
	{
		enforce!RpcException(type == RpcType.Integer || type == RpcType.UInteger,
								"RpcValue is not an integer");
		if(type == RpcType.UInteger) {
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
	 * Value getter/setter for `RpcType.UInteger`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `RpcType.UInteger`.
	 */
	ulong uinteger() const pure @safe
	{
		enforce!RpcException(type == RpcType.Integer || type == RpcType.UInteger,
								"RpcValue is not an unsigned integer");
		if(type == RpcType.Integer) {
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
	 * Value getter/setter for `RpcType.Float`. Note that despite
	 * the name, this is a $(B 64)-bit `double`, not a 32-bit `float`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `RpcType.Float`.
	 */
	double floating() const pure @safe
	{
		enforce!RpcException(type == RpcType.Float,
								"RpcValue is not a floating type");
		return store.floating;
	}

	double floating(double v) pure nothrow @safe @nogc
	{
		assign(v);
		return store.floating;
	}

	/***
	 * Value getter/setter for boolean stored in Rpc.
	 * Throws: `RpcException` for read access if `this.type` is not
	 * `RpcType.true_` or `RpcType.false_`.
	 */
	bool boolean() const pure @safe
	{
		if (type == RpcType.Bool) return store.boolean;
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
		if (type == RpcType.Decimal)
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
		if (type == RpcType.DateTime)
			return store.datetime;
		throw new RpcException("RpcValue is not a datetime type");
	}

	Decimal decimal(Decimal v) pure nothrow @safe @nogc
	{
		assign(v);
		return store.decimal;
	}

	/***
	 * Value getter/setter for `RpcType.Map`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `RpcType.Map`.
	 * Note: this is @system because of the following pattern:
	   ---
	   auto a = &(json.object());
	   json.uinteger = 0;        // overwrite AA pointer
	   (*a)["hello"] = "world";  // segmentation fault
	   ---
	 */
	ref inout(RpcValue[string]) map() inout pure @system
	{
		enforce!RpcException(type == RpcType.Map,
								"RpcValue is not an object");
		return store.map;
	}

	RpcValue[string] map(RpcValue[string] v) pure nothrow @nogc @safe
	{
		assign(v);
		return v;
	}

	/***
	 * Value getter for `RpcType.Map`.
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
	 * `RpcType.Map`.
	 */
	inout(RpcValue[string]) mapNoRef() inout pure @trusted
	{
		enforce!RpcException(type == RpcType.Map,
								"RpcValue is not an map");
		return store.map;
	}

	ref inout(RpcValue[int]) imap() inout pure @system
	{
		enforce!RpcException(type == RpcType.IMap,
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
		enforce!RpcException(type == RpcType.IMap,
								"RpcValue is not an IMap");
		return store.imap;
	}

	/***
	 * Value getter/setter for `RpcType.List`.
	 * Throws: `RpcException` for read access if `type` is not
	 * `RpcType.List`.
	 * Note: this is @system because of the following pattern:
	   ---
	   auto a = &(json.array());
	   json.uinteger = 0;  // overwrite array pointer
	   (*a)[0] = "world";  // segmentation fault
	   ---
	 */
	ref inout(RpcValue[]) list() inout pure @system
	{
		enforce!RpcException(type == RpcType.List,
								"RpcValue is not an list");
		return store.list;
	}

	RpcValue[] list(RpcValue[] v) pure nothrow @nogc @safe
	{
		assign(v);
		return v;
	}

	/***
	 * Value getter for `RpcType.List`.
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
	 * `RpcType.List`.
	 */
	inout(RpcValue[]) listNoRef() inout pure @trusted
	{
		enforce!RpcException(type == RpcType.List,
								"RpcValue is not an list");
		return store.list;
	}

	/// Test whether the type is `RpcType.null_`
	bool isNull() const pure nothrow @safe @nogc
	{
		return type == RpcType.Null;
	}

	private void assign(T)(T arg) @safe
	{
		static if (is(T : typeof(null)))
		{
			type_tag = RpcType.Null;
		}
		else static if (is(T : bool))
		{
			type_tag = RpcType.Bool;
			bool t = arg;
			() @trusted { store.boolean = t; }();
		}
		else static if (is(T : string))
		{
			type_tag = RpcType.String;
			string t = arg;
			() @trusted { store.str = t; }();
		}
		else static if (is(T : Decimal))
		{
			type_tag = RpcType.Decimal;
			Decimal t = arg;
			() @trusted { store.decimal = t; }();
		}
		else static if (isSomeString!T) // issue 15884
		{
			type_tag = RpcType.String;
			// FIXME: std.Array.Array(Range) is not deduced as 'pure'
			() @trusted {
				import std.utf : byUTF;
				store.str = cast(immutable)(arg.byUTF!char.array);
			}();
		}
		else static if (is(T : bool))
		{
			type_tag = arg ? RpcType.true_ : RpcType.false_;
		}
		else static if (is(T : ulong) && isUnsigned!T)
		{
			type_tag = RpcType.UInteger;
			store.uinteger = arg;
		}
		else static if (is(T : long))
		{
			type_tag = RpcType.Integer;
			store.integer = arg;
		}
		else static if (isFloatingPoint!T)
		{
			type_tag = RpcType.Float;
			store.floating = arg;
		}
		else static if (is(T : Decimal)) {
			type_tag = RpcType.Decimal;
			store.datetime = arg;
		}
		else static if (is(T : DateTime)) {
			type_tag = RpcType.DateTime;
			store.datetime = arg;
		}
		else static if (is(T : SysTime)) {
			type_tag = RpcType.DateTime;
			store.datetime = DateTime.fromSysTime(arg);
		}
		else static if (is(T : Value[Key], Key, Value))
		{
			static if(is(Key : string)) {
				type_tag = RpcType.Map;
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
				type_tag = RpcType.IMap;
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
			type_tag = RpcType.List;
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
		type_tag = RpcType.List;
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
	 * Note that this is a shallow copy: if type is `RpcType.Map`
	 * or `RpcType.List` then only the reference to the data will
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
		assert(j.type == RpcType.List);

		j = RpcValue( ["language": "D"] );
		assert(j.type == RpcType.Map);
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
	 * Throws: `RpcException` if `type` is not `RpcType.List`.
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
	 * Throws: `RpcException` if `type` is not `RpcType.Map`.
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
	 * Throws: `RpcException` if `type` is not `RpcType.Map`
	 * or `RpcType.null_`.
	 */
	void opIndexAssign(T)(auto ref T value, string key) pure
	{
		enforce!RpcException(type == RpcType.Map || type == RpcType.Null,
								"RpcValue must be object or null");
		RpcValue[string] aa = null;
		if (type == RpcType.Map) {
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
		case RpcType.Invalid:
			switch (rhs.type_tag)
			{
				case RpcType.Invalid:
					return true;
				default:
					return false;
			}
		case RpcType.Integer:
			switch (rhs.type_tag)
			{
				case RpcType.Integer:
					return store.integer == rhs.store.integer;
				case RpcType.UInteger:
					return store.integer == rhs.store.uinteger;
				case RpcType.Float:
					return store.integer == rhs.store.floating;
				default:
					return false;
			}
		case RpcType.UInteger:
			switch (rhs.type_tag)
			{
				case RpcType.Integer:
					return store.uinteger == rhs.store.integer;
				case RpcType.UInteger:
					return store.uinteger == rhs.store.uinteger;
				case RpcType.Float:
					return store.uinteger == rhs.store.floating;
				default:
					return false;
			}
		case RpcType.Float:
			switch (rhs.type_tag)
			{
				case RpcType.Integer:
					return store.floating == rhs.store.integer;
				case RpcType.UInteger:
					return store.floating == rhs.store.uinteger;
				case RpcType.Float:
					return store.floating == rhs.store.floating;
				default:
					return false;
			}
		case RpcType.Bool:
			return type_tag == rhs.type_tag && store.boolean == rhs.store.boolean;
		case RpcType.Decimal:
			return type_tag == rhs.type_tag && store.decimal == rhs.store.decimal;
		case RpcType.DateTime:
			return type_tag == rhs.type_tag && store.datetime == rhs.store.datetime;
		case RpcType.String:
			return type_tag == rhs.type_tag && store.str == rhs.store.str;
		case RpcType.Map:
			return type_tag == rhs.type_tag && store.map == rhs.store.map;
		case RpcType.IMap:
			return type_tag == rhs.type_tag && store.imap == rhs.store.imap;
		case RpcType.List:
			return type_tag == rhs.type_tag && store.list == rhs.store.list;
		case RpcType.Null:
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
		enforce!RpcException(type == RpcType.Map,
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

	/***
	 * Implicitly calls `toRpc` on this RpcValue.
	 *
	 * $(I options) can be used to tweak the conversion behavior.
	 */
	string toCpon(Flag!"pretty" pretty = No.pretty) const @safe
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
	static RpcValue fromCpon(string cpon) @safe
	{
		import shv.cpon : parse;
		return parse(cpon);
	}
}

/**
Parses a serialized string and returns a tree of Rpc values.
Throws: $(LREF RpcException) if string does not follow the Rpc grammar or the depth exceeds the max depth,
		$(LREF ConvException) if a number in the input cannot be represented by a native D type.
Params:
	json = json-formatted string to parse
	maxDepth = maximum depth of nesting allowed, -1 disables depth checking
	options = enable decoding string representations of NaN/Inf as float values
*/
RpcValue parseRpc(T)(T json, int maxDepth = -1, RpcOptions options = RpcOptions.none)
if (isInputRange!T && !isInfinite!T && isSomeChar!(ElementEncodingType!T))
{
	import std.ascii : isDigit, isHexDigit, toUpper, toLower;
	import std.typecons : Nullable, Yes;
	RpcValue root;
	root.type_tag = RpcType.null_;

	// Avoid UTF decoding when possible, as it is unnecessary when
	// processing Rpc.
	static if (is(T : const(char)[]))
		alias Char = char;
	else
		alias Char = Unqual!(ElementType!T);

	int depth = -1;
	Nullable!Char next;
	int line = 1, pos = 0;
	immutable bool strict = (options & RpcOptions.strictParsing) != 0;

	void error(string msg)
	{
		throw new RpcException(msg, line, pos);
	}

	if (json.empty)
	{
		if (strict)
		{
			error("Empty Rpc body");
		}
		return root;
	}

	bool isWhite(dchar c)
	{
		if (strict)
		{
			// RFC 7159 has a stricter definition of whitespace than general ASCII.
			return c == ' ' || c == '\t' || c == '\n' || c == '\r';
		}
		import std.ascii : isWhite;
		// Accept ASCII NUL as whitespace in non-strict mode.
		return c == 0 || isWhite(c);
	}

	Char popChar()
	{
		if (json.empty) error("Unexpected end of data.");
		static if (is(T : const(char)[]))
		{
			Char c = json[0];
			json = json[1..$];
		}
		else
		{
			Char c = json.front;
			json.popFront();
		}

		if (c == '\n')
		{
			line++;
			pos = 0;
		}
		else
		{
			pos++;
		}

		return c;
	}

	Char peekChar()
	{
		if (next.isNull)
		{
			if (json.empty) return '\0';
			next = popChar();
		}
		return next.get;
	}

	Nullable!Char peekCharNullable()
	{
		if (next.isNull && !json.empty)
		{
			next = popChar();
		}
		return next;
	}

	void skipWhitespace()
	{
		while (true)
		{
			auto c = peekCharNullable();
			if (c.isNull ||
				!isWhite(c.get))
			{
				return;
			}
			next.nullify();
		}
	}

	Char getChar(bool SkipWhitespace = false)()
	{
		static if (SkipWhitespace) skipWhitespace();

		Char c;
		if (!next.isNull)
		{
			c = next.get;
			next.nullify();
		}
		else
			c = popChar();

		return c;
	}

	void checkChar(bool SkipWhitespace = true)(char c, bool caseSensitive = true)
	{
		static if (SkipWhitespace) skipWhitespace();
		auto c2 = getChar();
		if (!caseSensitive) c2 = toLower(c2);

		if (c2 != c) error(text("Found '", c2, "' when expecting '", c, "'."));
	}

	bool testChar(bool SkipWhitespace = true, bool CaseSensitive = true)(char c)
	{
		static if (SkipWhitespace) skipWhitespace();
		auto c2 = peekChar();
		static if (!CaseSensitive) c2 = toLower(c2);

		if (c2 != c) return false;

		getChar();
		return true;
	}

	wchar parseWChar()
	{
		wchar val = 0;
		foreach_reverse (i; 0 .. 4)
		{
			auto hex = toUpper(getChar());
			if (!isHexDigit(hex)) error("Expecting hex character");
			val += (isDigit(hex) ? hex - '0' : hex - ('A' - 10)) << (4 * i);
		}
		return val;
	}

	string parseString()
	{
		import std.uni : isSurrogateHi, isSurrogateLo;
		import std.utf : encode, decode;

		auto str = appender!string();

	Next:
		switch (peekChar())
		{
			case '"':
				getChar();
				break;

			case '\\':
				getChar();
				auto c = getChar();
				switch (c)
				{
					case '"':       str.put('"');   break;
					case '\\':      str.put('\\');  break;
					case '/':       str.put('/');   break;
					case 'b':       str.put('\b');  break;
					case 'f':       str.put('\f');  break;
					case 'n':       str.put('\n');  break;
					case 'r':       str.put('\r');  break;
					case 't':       str.put('\t');  break;
					case 'u':
						wchar wc = parseWChar();
						dchar val;
						// Non-BMP characters are escaped as a pair of
						// UTF-16 surrogate characters (see RFC 4627).
						if (isSurrogateHi(wc))
						{
							wchar[2] pair;
							pair[0] = wc;
							if (getChar() != '\\') error("Expected escaped low surrogate after escaped high surrogate");
							if (getChar() != 'u') error("Expected escaped low surrogate after escaped high surrogate");
							pair[1] = parseWChar();
							size_t index = 0;
							val = decode(pair[], index);
							if (index != 2) error("Invalid escaped surrogate pair");
						}
						else
						if (isSurrogateLo(wc))
							error(text("Unexpected low surrogate"));
						else
							val = wc;

						char[4] buf;
						immutable len = encode!(Yes.useReplacementDchar)(buf, val);
						str.put(buf[0 .. len]);
						break;

					default:
						error(text("Invalid escape sequence '\\", c, "'."));
				}
				goto Next;

			default:
				// RFC 7159 states that control characters U+0000 through
				// U+001F must not appear unescaped in a Rpc string.
				// Note: std.ascii.isControl can't be used for this test
				// because it considers ASCII DEL (0x7f) to be a control
				// character but RFC 7159 does not.
				// Accept unescaped ASCII NULs in non-strict mode.
				auto c = getChar();
				if (c < 0x20 && (strict || c != 0))
					error("Illegal control character.");
				str.put(c);
				goto Next;
		}

		return str.data.length ? str.data : "";
	}

	bool tryGetSpecialFloat(string str, out double val) {
		switch (str)
		{
			case RpcFloatLiteral.nan:
				val = double.nan;
				return true;
			case RpcFloatLiteral.inf:
				val = double.infinity;
				return true;
			case RpcFloatLiteral.negativeInf:
				val = -double.infinity;
				return true;
			default:
				return false;
		}
	}

	void parseValue(ref RpcValue value)
	{
		depth++;

		if (maxDepth != -1 && depth > maxDepth) error("Nesting too deep.");

		auto c = getChar!true();

		switch (c)
		{
			case '{':
				if (testChar('}'))
				{
					value.object = null;
					break;
				}

				RpcValue[string] obj;
				do
				{
					skipWhitespace();
					if (!strict && peekChar() == '}')
					{
						break;
					}
					checkChar('"');
					string name = parseString();
					checkChar(':');
					RpcValue member;
					parseValue(member);
					obj[name] = member;
				}
				while (testChar(','));
				value.object = obj;

				checkChar('}');
				break;

			case '[':
				if (testChar(']'))
				{
					value.type_tag = RpcType.List;
					break;
				}

				RpcValue[] arr;
				do
				{
					skipWhitespace();
					if (!strict && peekChar() == ']')
					{
						break;
					}
					RpcValue element;
					parseValue(element);
					arr ~= element;
				}
				while (testChar(','));

				checkChar(']');
				value.array = arr;
				break;

			case '"':
				auto str = parseString();

				// if special float parsing is enabled, check if string represents NaN/Inf
				if ((options & RpcOptions.specialFloatLiterals) &&
					tryGetSpecialFloat(str, value.store.floating))
				{
					// found a special float, its value was placed in value.store.floating
					value.type_tag = RpcType.Float;
					break;
				}

				value.type_tag = RpcType.String;
				value.store.str = str;
				break;

			case '0': .. case '9':
			case '-':
				auto number = appender!string();
				bool isFloat, isNegative;

				void readInteger()
				{
					if (!isDigit(c)) error("Digit expected");

				Next: number.put(c);

					if (isDigit(peekChar()))
					{
						c = getChar();
						goto Next;
					}
				}

				if (c == '-')
				{
					number.put('-');
					c = getChar();
					isNegative = true;
				}

				if (strict && c == '0')
				{
					number.put('0');
					if (isDigit(peekChar()))
					{
						error("Additional digits not allowed after initial zero digit");
					}
				}
				else
				{
					readInteger();
				}

				if (testChar('.'))
				{
					isFloat = true;
					number.put('.');
					c = getChar();
					readInteger();
				}
				if (testChar!(false, false)('e'))
				{
					isFloat = true;
					number.put('e');
					if (testChar('+')) number.put('+');
					else if (testChar('-')) number.put('-');
					c = getChar();
					readInteger();
				}

				string data = number.data;
				if (isFloat)
				{
					value.type_tag = RpcType.Float;
					value.store.floating = parse!double(data);
				}
				else
				{
					if (isNegative)
						value.store.integer = parse!long(data);
					else
						value.store.uinteger = parse!ulong(data);

					value.type_tag = !isNegative && value.store.uinteger & (1UL << 63) ?
						RpcType.UInteger : RpcType.Integer;
				}
				break;

			case 'T':
				if (strict) goto default;
				goto case;
			case 't':
				value.type_tag = RpcType.Bool;
				value.store.boolean = true;
				checkChar!false('r', strict);
				checkChar!false('u', strict);
				checkChar!false('e', strict);
				break;

			case 'F':
				if (strict) goto default;
				goto case;
			case 'f':
				value.type_tag = RpcType.Bool;
				value.store.boolean = false;
				checkChar!false('a', strict);
				checkChar!false('l', strict);
				checkChar!false('s', strict);
				checkChar!false('e', strict);
				break;

			case 'N':
				if (strict) goto default;
				goto case;
			case 'n':
				value.type_tag = RpcType.null_;
				checkChar!false('u', strict);
				checkChar!false('l', strict);
				checkChar!false('l', strict);
				break;

			default:
				error(text("Unexpected character '", c, "'."));
		}

		depth--;
	}

	parseValue(root);
	if (strict)
	{
		skipWhitespace();
		if (!peekCharNullable().isNull) error("Trailing non-whitespace characters");
	}
	return root;
}

/+ @safe unittest
{
	enum issue15742objectOfObject = `{ "key1": { "key2": 1 }}`;
	static assert(parseRpc(issue15742objectOfObject).type == RpcType.Map);

	enum issue15742arrayOfArray = `[[1]]`;
	static assert(parseRpc(issue15742arrayOfArray).type == RpcType.List);
}
+/

/+ @safe unittest
{
	// Ensure we can parse and use Rpc from @safe code
	auto a = `{ "key1": { "key2": 1 }}`.parseRpc;
	assert(a["key1"]["key2"].integer == 1);
	assert(a.toString == `{"key1":{"key2":1}}`);
}
+/

/+ @system unittest
{
	// Ensure we can parse Rpc from a @system range.
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
Parses a serialized string and returns a tree of Rpc values.
Throws: $(LREF RpcException) if the depth exceeds the max depth.
Params:
	json = json-formatted string to parse
	options = enable decoding string representations of NaN/Inf as float values
*/
RpcValue parseRpc(T)(T json, RpcOptions options)
if (isInputRange!T && !isInfinite!T && isSomeChar!(ElementEncodingType!T))
{
	return parseRpc!T(json, -1, options);
}

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


/+ @system unittest
{
	import std.exception;
	RpcValue jv = "123";
	assert(jv.type == RpcType.String);
	assertNotThrown(jv.str);
	assertThrown!RpcException(jv.integer);
	assertThrown!RpcException(jv.uinteger);
	assertThrown!RpcException(jv.floating);
	assertThrown!RpcException(jv.object);
	assertThrown!RpcException(jv.array);
	assertThrown!RpcException(jv["aa"]);
	assertThrown!RpcException(jv[2]);

	jv = -3;
	assert(jv.type == RpcType.Integer);
	assertNotThrown(jv.integer);

	jv = cast(uint) 3;
	assert(jv.type == RpcType.UInteger);
	assertNotThrown(jv.uinteger);

	jv = 3.0;
	assert(jv.type == RpcType.Float);
	assertNotThrown(jv.floating);

	jv = ["key" : "value"];
	assert(jv.type == RpcType.Map);
	assertNotThrown(jv.object);
	assertNotThrown(jv["key"]);
	assert("key" in jv);
	assert("notAnElement" !in jv);
	assertThrown!RpcException(jv["notAnElement"]);
	const cjv = jv;
	assert("key" in cjv);
	assertThrown!RpcException(cjv["notAnElement"]);

	foreach (string key, value; jv)
	{
		static assert(is(typeof(value) == RpcValue));
		assert(key == "key");
		assert(value.type == RpcType.String);
		assertNotThrown(value.str);
		assert(value.str == "value");
	}

	jv = [3, 4, 5];
	assert(jv.type == RpcType.List);
	assertNotThrown(jv.array);
	assertNotThrown(jv[2]);
	foreach (size_t index, value; jv)
	{
		static assert(is(typeof(value) == RpcValue));
		assert(value.type == RpcType.Integer);
		assertNotThrown(value.integer);
		assert(index == (value.integer-3));
	}

	jv = null;
	assert(jv.type == RpcType.null_);
	assert(jv.isNull);
	jv = "foo";
	assert(!jv.isNull);

	jv = RpcValue("value");
	assert(jv.type == RpcType.String);
	assert(jv.str == "value");

	RpcValue jv2 = RpcValue("value");
	assert(jv2.type == RpcType.String);
	assert(jv2.str == "value");

	RpcValue jv3 = RpcValue("\u001c");
	assert(jv3.type == RpcType.String);
	assert(jv3.str == "\u001C");
}
+/

/+ @system unittest
{
	// Bugzilla 11504

	RpcValue jv = 1;
	assert(jv.type == RpcType.Integer);

	jv.str = "123";
	assert(jv.type == RpcType.String);
	assert(jv.str == "123");

	jv.integer = 1;
	assert(jv.type == RpcType.Integer);
	assert(jv.integer == 1);

	jv.uinteger = 2u;
	assert(jv.type == RpcType.UInteger);
	assert(jv.uinteger == 2u);

	jv.floating = 1.5;
	assert(jv.type == RpcType.Float);
	assert(jv.floating == 1.5);

	jv.object = ["key" : RpcValue("value")];
	assert(jv.type == RpcType.Map);
	assert(jv.object == ["key" : RpcValue("value")]);

	jv.array = [RpcValue(1), RpcValue(2), RpcValue(3)];
	assert(jv.type == RpcType.List);
	assert(jv.array == [RpcValue(1), RpcValue(2), RpcValue(3)]);

	jv = true;
	assert(jv.type == RpcType.true_);

	jv = false;
	assert(jv.type == RpcType.false_);

	enum E{True = true}
	jv = E.True;
	assert(jv.type == RpcType.true_);
}
+/

/+ @system pure unittest
{
	// Adding new json element via array() / object() directly

	RpcValue jarr = RpcValue([10]);
	foreach (i; 0 .. 9)
		jarr.array ~= RpcValue(i);
	assert(jarr.array.length == 10);

	RpcValue jobj = RpcValue(["key" : RpcValue("value")]);
	foreach (i; 0 .. 9)
		jobj.object[text("key", i)] = RpcValue(text("value", i));
	assert(jobj.object.length == 10);
}
+/

/+ @system pure unittest
{
	// Adding new json element without array() / object() access

	RpcValue jarr = RpcValue([10]);
	foreach (i; 0 .. 9)
		jarr ~= [RpcValue(i)];
	assert(jarr.array.length == 10);

	RpcValue jobj = RpcValue(["key" : RpcValue("value")]);
	foreach (i; 0 .. 9)
		jobj[text("key", i)] = RpcValue(text("value", i));
	assert(jobj.object.length == 10);

	// No array alias
	auto jarr2 = jarr ~ [1,2,3];
	jarr2[0] = 999;
	assert(jarr[0] == RpcValue(10));
}
+/

/+ @system unittest
{
	// @system because RpcValue.array is @system
	import std.exception;

	// An overly simple test suite, if it can parse a serializated string and
	// then use the resulting values tree to generate an identical
	// serialization, both the decoder and encoder works.

	auto jsons = [
		`null`,
		`true`,
		`false`,
		`0`,
		`123`,
		`-4321`,
		`0.25`,
		`-0.25`,
		`""`,
		`"hello\nworld"`,
		`"\"\\\/\b\f\n\r\t"`,
		`[]`,
		`[12,"foo",true,false]`,
		`{}`,
		`{"a":1,"b":null}`,
		`{"goodbye":[true,"or",false,["test",42,{"nested":{"a":23.5,"b":0.140625}}]],`
		~`"hello":{"array":[12,null,{}],"json":"is great"}}`,
	];

	enum dbl1_844 = `1.8446744073709568`;
	version (MinGW)
		jsons ~= dbl1_844 ~ `e+019`;
	else
		jsons ~= dbl1_844 ~ `e+19`;

	RpcValue val;
	string result;
	foreach (json; jsons)
	{
		try
		{
			val = parseRpc(json);
			enum pretty = false;
			result = toRpc(val, pretty);
			assert(result == json, text(result, " should be ", json));
		}
		catch (RpcException e)
		{
			import std.stdio : writefln;
			writefln(text(json, "\n", e.toString()));
		}
	}

	// Should be able to correctly interpret unicode entities
	val = parseRpc(`"\u003C\u003E"`);
	assert(toRpc(val) == "\"\&lt;\&gt;\"");
	assert(val.to!string() == "\"\&lt;\&gt;\"");
	val = parseRpc(`"\u0391\u0392\u0393"`);
	assert(toRpc(val) == "\"\&Alpha;\&Beta;\&Gamma;\"");
	assert(val.to!string() == "\"\&Alpha;\&Beta;\&Gamma;\"");
	val = parseRpc(`"\u2660\u2666"`);
	assert(toRpc(val) == "\"\&spades;\&diams;\"");
	assert(val.to!string() == "\"\&spades;\&diams;\"");

	//0x7F is a control character (see Unicode spec)
	val = parseRpc(`"\u007F"`);
	assert(toRpc(val) == "\"\\u007F\"");
	assert(val.to!string() == "\"\\u007F\"");

	with(parseRpc(`""`))
		assert(str == "" && str !is null);
	with(parseRpc(`[]`))
		assert(!array.length);

	// Formatting
	val = parseRpc(`{"a":[null,{"x":1},{},[]]}`);
	assert(toRpc(val, true) == `{
	"a": [
		null,
		{
			"x": 1
		},
		{},
		[]
	]
}`);
}
+/

/+ @safe unittest
{
  auto json = `"hello\nworld"`;
  const jv = parseRpc(json);
  assert(jv.toString == json);
  assert(jv.toPrettyString == json);
}
+/

/+ @system pure unittest
{
	// Bugzilla 12969

	RpcValue jv;
	jv["int"] = 123;

	assert(jv.type == RpcType.Map);
	assert("int" in jv);
	assert(jv["int"].integer == 123);

	jv["array"] = [1, 2, 3, 4, 5];

	assert(jv["array"].type == RpcType.List);
	assert(jv["array"][2].integer == 3);

	jv["str"] = "D language";
	assert(jv["str"].type == RpcType.String);
	assert(jv["str"].str == "D language");

	jv["bool"] = false;
	assert(jv["bool"].type == RpcType.false_);

	assert(jv.object.length == 4);

	jv = [5, 4, 3, 2, 1];
	assert(jv.type == RpcType.List);
	assert(jv[3].integer == 2);
}
+/

/+ @safe unittest
{
	auto s = q"EOF
[
  1,
  2,
  3,
  potato
]
EOF";

	import std.exception;

	auto e = collectException!RpcException(parseRpc(s));
	assert(e.msg == "Unexpected character 'p'. (Line 5:3)", e.msg);
}
+/

// handling of special float values (NaN, Inf, -Inf)
/+ @safe unittest
{
	import std.exception : assertThrown;
	import std.math : isNaN, isInfinity;

	// expected representations of NaN and Inf
	enum {
		nanString         = '"' ~ RpcFloatLiteral.nan         ~ '"',
		infString         = '"' ~ RpcFloatLiteral.inf         ~ '"',
		negativeInfString = '"' ~ RpcFloatLiteral.negativeInf ~ '"',
	}

	// with the specialFloatLiterals option, encode NaN/Inf as strings
	assert(RpcValue(float.nan).toString(RpcOptions.specialFloatLiterals)       == nanString);
	assert(RpcValue(double.infinity).toString(RpcOptions.specialFloatLiterals) == infString);
	assert(RpcValue(-real.infinity).toString(RpcOptions.specialFloatLiterals)  == negativeInfString);

	// without the specialFloatLiterals option, throw on encoding NaN/Inf
	assertThrown!RpcException(RpcValue(float.nan).toString);
	assertThrown!RpcException(RpcValue(double.infinity).toString);
	assertThrown!RpcException(RpcValue(-real.infinity).toString);

	// when parsing json with specialFloatLiterals option, decode special strings as floats
	RpcValue jvNan    = parseRpc(nanString, RpcOptions.specialFloatLiterals);
	RpcValue jvInf    = parseRpc(infString, RpcOptions.specialFloatLiterals);
	RpcValue jvNegInf = parseRpc(negativeInfString, RpcOptions.specialFloatLiterals);

	assert(jvNan.floating.isNaN);
	assert(jvInf.floating.isInfinity    && jvInf.floating > 0);
	assert(jvNegInf.floating.isInfinity && jvNegInf.floating < 0);

	// when parsing json without the specialFloatLiterals option, decode special strings as strings
	jvNan    = parseRpc(nanString);
	jvInf    = parseRpc(infString);
	jvNegInf = parseRpc(negativeInfString);

	assert(jvNan.str    == RpcFloatLiteral.nan);
	assert(jvInf.str    == RpcFloatLiteral.inf);
	assert(jvNegInf.str == RpcFloatLiteral.negativeInf);
}
+/

/+ pure nothrow @safe @nogc unittest
{
	RpcValue testVal;
	testVal = "test";
	testVal = 10;
	testVal = 10u;
	testVal = 1.0;
	testVal = (RpcValue[string]).init;
	testVal = RpcValue[].init;
	testVal = null;
	assert(testVal.isNull);
}
+/

/+ pure nothrow @safe unittest // issue 15884
{
	import std.typecons;
	void Test(C)() {
		C[] a = ['x'];
		RpcValue testVal = a;
		assert(testVal.type == RpcType.String);
		testVal = a.idup;
		assert(testVal.type == RpcType.String);
	}
	Test!char();
	Test!wchar();
	Test!dchar();
}
+/

/+ @safe unittest // issue 15885
{
	enum bool realInDoublePrecision = real.mant_dig == double.mant_dig;

	static bool test(const double num0)
	{
		import std.math : feqrel;
		const json0 = RpcValue(num0);
		const num1 = to!double(toRpc(json0));
		static if (realInDoublePrecision)
			return feqrel(num1, num0) >= (double.mant_dig - 1);
		else
			return num1 == num0;
	}

	assert(test( 0.23));
	assert(test(-0.23));
	assert(test(1.223e+24));
	assert(test(23.4));
	assert(test(0.0012));
	assert(test(30738.22));

	assert(test(1 + double.epsilon));
	assert(test(double.min_normal));
	static if (realInDoublePrecision)
		assert(test(-double.max / 2));
	else
		assert(test(-double.max));

	const minSub = double.min_normal * double.epsilon;
	assert(test(minSub));
	assert(test(3*minSub));
}
+/

/+ @safe unittest // issue 17555
{
	import std.exception : assertThrown;

	assertThrown!RpcException(parseRpc("\"a\nb\""));
}
+/

/+ @safe unittest // issue 17556
{
	auto v = RpcValue("\U0001D11E");
	auto j = toRpc(v, false, RpcOptions.escapeNonAsciiChars);
	assert(j == `"\uD834\uDD1E"`);
}
+/

/+ @safe unittest // issue 5904
{
	string s = `"\uD834\uDD1E"`;
	auto j = parseRpc(s);
	assert(j.str == "\U0001D11E");
}
+/

/+ @safe unittest // issue 17557
{
	assert(parseRpc("\"\xFF\"").str == "\xFF");
	assert(parseRpc("\"\U0001D11E\"").str == "\U0001D11E");
}
+/

/+ @safe unittest // issue 17553
{
	auto v = RpcValue("\xFF");
	assert(toRpc(v) == "\"\xFF\"");
}
+/

/+ @safe unittest
{
	import std.utf;
	assert(parseRpc("\"\xFF\"".byChar).str == "\xFF");
	assert(parseRpc("\"\U0001D11E\"".byChar).str == "\U0001D11E");
}
+/

/+ @safe unittest // RpcOptions.doNotEscapeSlashes (issue 17587)
{
	assert(parseRpc(`"/"`).toString == `"\/"`);
	assert(parseRpc(`"\/"`).toString == `"\/"`);
	assert(parseRpc(`"/"`).toString(RpcOptions.doNotEscapeSlashes) == `"/"`);
	assert(parseRpc(`"\/"`).toString(RpcOptions.doNotEscapeSlashes) == `"/"`);
}
+/

/+ @safe unittest // RpcOptions.strictParsing (issue 16639)
{
	import std.exception : assertThrown;

	// Unescaped ASCII NULs
	assert(parseRpc("[\0]").type == RpcType.List);
	assertThrown!RpcException(parseRpc("[\0]", RpcOptions.strictParsing));
	assert(parseRpc("\"\0\"").str == "\0");
	assertThrown!RpcException(parseRpc("\"\0\"", RpcOptions.strictParsing));

	// Unescaped ASCII DEL (0x7f) in strings
	assert(parseRpc("\"\x7f\"").str == "\x7f");
	assert(parseRpc("\"\x7f\"", RpcOptions.strictParsing).str == "\x7f");

	// "true", "false", "null" case sensitivity
	assert(parseRpc("true").type == RpcType.true_);
	assert(parseRpc("true", RpcOptions.strictParsing).type == RpcType.true_);
	assert(parseRpc("True").type == RpcType.true_);
	assertThrown!RpcException(parseRpc("True", RpcOptions.strictParsing));
	assert(parseRpc("tRUE").type == RpcType.true_);
	assertThrown!RpcException(parseRpc("tRUE", RpcOptions.strictParsing));

	assert(parseRpc("false").type == RpcType.false_);
	assert(parseRpc("false", RpcOptions.strictParsing).type == RpcType.false_);
	assert(parseRpc("False").type == RpcType.false_);
	assertThrown!RpcException(parseRpc("False", RpcOptions.strictParsing));
	assert(parseRpc("fALSE").type == RpcType.false_);
	assertThrown!RpcException(parseRpc("fALSE", RpcOptions.strictParsing));

	assert(parseRpc("null").type == RpcType.null_);
	assert(parseRpc("null", RpcOptions.strictParsing).type == RpcType.null_);
	assert(parseRpc("Null").type == RpcType.null_);
	assertThrown!RpcException(parseRpc("Null", RpcOptions.strictParsing));
	assert(parseRpc("nULL").type == RpcType.null_);
	assertThrown!RpcException(parseRpc("nULL", RpcOptions.strictParsing));

	// Whitespace characters
	assert(parseRpc("[\f\v]").type == RpcType.List);
	assertThrown!RpcException(parseRpc("[\f\v]", RpcOptions.strictParsing));
	assert(parseRpc("[ \t\r\n]").type == RpcType.List);
	assert(parseRpc("[ \t\r\n]", RpcOptions.strictParsing).type == RpcType.List);

	// Empty input
	assert(parseRpc("").type == RpcType.null_);
	assertThrown!RpcException(parseRpc("", RpcOptions.strictParsing));

	// Numbers with leading '0's
	assert(parseRpc("01").integer == 1);
	assertThrown!RpcException(parseRpc("01", RpcOptions.strictParsing));
	assert(parseRpc("-01").integer == -1);
	assertThrown!RpcException(parseRpc("-01", RpcOptions.strictParsing));
	assert(parseRpc("0.01").floating == 0.01);
	assert(parseRpc("0.01", RpcOptions.strictParsing).floating == 0.01);
	assert(parseRpc("0e1").floating == 0);
	assert(parseRpc("0e1", RpcOptions.strictParsing).floating == 0);

	// Trailing characters after Rpc value
	assert(parseRpc(`""asdf`).str == "");
	assertThrown!RpcException(parseRpc(`""asdf`, RpcOptions.strictParsing));
	assert(parseRpc("987\0").integer == 987);
	assertThrown!RpcException(parseRpc("987\0", RpcOptions.strictParsing));
	assert(parseRpc("987\0\0").integer == 987);
	assertThrown!RpcException(parseRpc("987\0\0", RpcOptions.strictParsing));
	assert(parseRpc("[]]").type == RpcType.List);
	assertThrown!RpcException(parseRpc("[]]", RpcOptions.strictParsing));
	assert(parseRpc("123 \t\r\n").integer == 123); // Trailing whitespace is OK
	assert(parseRpc("123 \t\r\n", RpcOptions.strictParsing).integer == 123);
}
+/

/+ @system unittest
{
	import std.algorithm.iteration : map;
	import std.array : array;
	import std.exception : assertThrown;

	string s = `{ "a" : [1,2,3,], }`;
	RpcValue j = parseRpc(s);
	assert(j["a"].array().map!(i => i.integer()).array == [1,2,3]);

	assertThrown(parseRpc(s, -1, RpcOptions.strictParsing));
}
+/

/+ @system unittest
{
	import std.algorithm.iteration : map;
	import std.array : array;
	import std.exception : assertThrown;

	string s = `{ "a" : { }  , }`;
	RpcValue j = parseRpc(s);
	assert("a" in j);
	auto t = j["a"].object();
	assert(t.empty);

	assertThrown(parseRpc(s, -1, RpcOptions.strictParsing));
}
+/
