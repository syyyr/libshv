import shv.rpcvalue;
import shv.cpon;
import shv.chainpack;
import shv.log;

import std.stdio;
import std.algorithm;
import std.typecons;
import std.conv;

alias log = logInfo;
alias logD = logMessage;

RpcValue fromChainPack(ubyte[] data)
{
	return shv.chainpack.read(data);
}

ubyte[] toChainPack(const ref RpcValue rv)
{
	return shv.chainpack.write(rv);
}

RpcValue fromCpon(string str)
{
	return shv.cpon.read(str);
}

string toCpon(const ref RpcValue rv)
{
	return shv.cpon.write(rv);
}

void test_vals()
{
	log("------------- NULL ");
	{
		auto rv = RpcValue(null);
		assert(rv.toCpon() == "null");
		assert(rv.toChainPack() == cast(ubyte[])[PackingSchema.Null]);
	}
	log("------------- BOOL ");
	foreach (bool b; cast(ubyte[])[true, false]) {
		auto rv = RpcValue(b);
		assert(rv.toCpon() == (b? "true": "false"));
		assert(rv.toChainPack() == (b? cast(ubyte[])[PackingSchema.TRUE]: cast(ubyte[])[PackingSchema.FALSE]));
	}
	log("------------- tiny uint ");
	for (ubyte n = 0; n < 64; ++n) {
		auto rv = RpcValue(n);
		assert(rv.toCpon() == (to!string(n) ~ 'u'));
		assert(rv.toChainPack() == cast(ubyte[])[n]);
	}
	log("------------- uint ");
	for (uint i = 0; i < ulong.sizeof; ++i) {
		for (uint j = 0; j < 3; ++j) {
			ulong n = ((cast(ulong)1) << (i*8 + j*3+1)) + 1;
			auto rv = RpcValue(n);
			auto cpon = rv.toCpon();
			auto cpk = rv.toChainPack();
			auto rv_cpon = fromCpon(cpon);
			auto rv_cpk = fromChainPack(cpk);
			logD(n, cpon, cpk);
			assert((to!string(n) ~ 'u') == cpon);
			assert(rv_cpon == rv_cpk);
			assert(rv_cpk.uinteger == n);
		}
	}
	log("------------- tiny int ");
	for (byte n = 0; n < 64; ++n) {
		auto rv = RpcValue(n);
		assert(rv.toCpon() == to!string(n));
		assert(rv.toChainPack() == cast(ubyte[])[n + 64]);
	}
	log("------------- int ");
	for (int sig = 1; sig >= -1; sig-=2) {
		for (uint i = 0; i < long.sizeof; ++i) {
			for (uint j = 0; j < 3; ++j) {
				long n = (sig * (cast(long)1) << (i*8 + j*2+2)) + 1;
				auto rv = RpcValue(n);
				auto cpon = rv.toCpon();
				auto cpk = rv.toChainPack();
				auto rv_cpon = fromCpon(cpon);
				auto rv_cpk = fromChainPack(cpk);
				logD(n, cpon, cpk);
				assert(to!string(n) == cpon);
				assert(rv_cpon == rv_cpk);
				assert(rv_cpk.integer == n);
			}
		}
	}
	log("------------- decimal ");
	{
		int mant = -123456;
		int exp_min = 1;
		int exp_max = 16;
		int step = 1;
		for (int exp = exp_min; exp <= exp_max; exp += step) {
			auto n = RpcValue.Decimal(mant, exp);
			RpcValue rv = n;
			auto cpon = rv.toCpon();
			auto cpk = rv.toChainPack();
			auto rv_cpon = fromCpon(cpon);
			auto rv_cpk = fromChainPack(cpk);
			logD(n, cpon, cpk);
			//logD(rv.decimal, rv.type, rv.decimal.mantisa, rv.decimal.exponent);
			//logD(rv_cpon.decimal, rv_cpon.type, rv_cpon.decimal.mantisa, rv_cpon.decimal.exponent);
			//logD(rv_cpk.decimal, rv_cpk.type, rv_cpk.decimal.mantisa, rv_cpk.decimal.exponent);
			assert(to!string(n) == cpon);
			assert(rv_cpk.decimal == n);
			assert(rv_cpon == rv_cpk);
		}
	}
	{
		log("------------- double");
		{
			double n_max = 1000000.;
			double n_min = -1000000.;
			double step = (n_max - n_min) / 100.1;
			for (double n = n_min; n < n_max; n += step) {
				RpcValue rv = n;
				auto cpon = rv.toCpon();
				auto cpk = rv.toChainPack();
				auto rv_cpk = fromChainPack(cpk);
				logD(n, cpon, cpk);
				assert(rv_cpk.floating == n);
			}
		}
		{
			double step = -1.23456789e10;
			for (double n = -double.max; n < double.max / -step / 10; n *= step) {
				RpcValue rv = n;
				auto cpon = rv.toCpon();
				auto cpk = rv.toChainPack();
				auto rv_cpon = fromCpon(cpon);
				auto rv_cpk = fromChainPack(cpk);
				logD(n, rv_cpk.floating, cpon, cpk);
				assert(rv_cpk.floating == n);
			}
		}
	}
	log("------------- datetime ");
	{
		auto cpons = [
			//"d\"\"", NULL,
			["d\"2018-02-02T00:00:00.001Z\"", "d\"2018-02-02T00:00:00.001Z\""],
			["d\"2018-02-02T01:00:00.001+01\"", "d\"2018-02-02T01:00:00.001+01\""],
			["d\"2018-12-02T00:00:00Z\"", "d\"2018-12-02T00:00:00Z\""],
			["d\"2041-03-04T00:00:00-1015\"", "d\"2041-03-04T00:00:00-1015\""],
			["d\"2041-03-04T00:00:00.123-1015\"", "d\"2041-03-04T00:00:00.123-1015\""],
			["d\"1970-01-01T00:00:00Z\"", "d\"1970-01-01T00:00:00Z\""],
			["d\"2017-05-03T05:52:03Z\"", "d\"2017-05-03T05:52:03Z\""],
			["d\"2017-05-03T15:52:03.923Z\"", "d\"2017-05-03T15:52:03.923Z\""],
			["d\"2017-05-03T15:52:03.000-0130\"", "d\"2017-05-03T15:52:03-0130\""],
			["d\"2017-05-03T15:52:03.923+00\"", "d\"2017-05-03T15:52:03.923Z\""],
		];
		foreach (cpon; cpons) {
			auto rv1 = fromCpon(cpon[0]);
			auto cpk = rv1.toChainPack();
			auto rv2 = fromChainPack(cpk);
			auto cpon2 = rv2.toCpon();
			log(cpon[0], cpon2, cpk);
			assert(cpon2 == cpon[1]);
		}
	}
	log("------------- cstring ");
	{
		auto cpons = [
			["", `""`],
			["hello", `"hello"`],
			["\t\r", "\"\\t\\r\""],
			["\\0", "\"\\\\0\""],
			["1\t\r\n\b", "\"1\\t\\r\\n\\b\""],
			["escaped zero \\0 here \t\r\n\b", "\"escaped zero \\\\0 here \\t\\r\\n\\b\""],
		];
		foreach (cpon; cpons) {
			RpcValue rv1 = cpon[0];
			auto cpk = rv1.toChainPack();
			auto rv2 = fromChainPack(cpk);
			auto cpon2 = rv2.toCpon();
			log(cpon[1], cpon2, cpk);
			assert(cpon2 == cpon[1]);
			auto rv3 = fromCpon(cpon2);
			assert(rv3.str == cpon[0]);
		}
	}
}

void testConversions()
{
	log("testConversions ------------");
	foreach(lst; [
		[uint.max.to!string ~ "u", null],
		[int.max.to!string, null],
		[int.min.to!string, null],
		[ulong.max.to!string ~ "u", null],
		[long.max.to!string, null],
		[long.min.to!string, null],
		["true", null],
		["false", null],
		["null", null],
		["1u", null],
		["134", null],
		["7", null],
		["-2", null],
		["0xab", "171"],
		["-0xCD", "-205"],
		["0x1a2b3c4d", "439041101"],
		["223.", null],
		["2.30", null],
		["12.3e-10", "123e-11"],
		["-0.00012", "-12e-5"],
		["-1234567890.", "-1234567890."],
		["\"foo\"", null],
		["[]", null],
		["[1]", null],
		["[1,]", "[1]"],
		["[1,2,3]", null],
		["[[]]", null],
		["{\"foo\":\"bar\"}", null],
		["i{1:2}", null],
		["i{\n\t1: \"bar\",\n\t345u : \"foo\",\n}", "i{1:\"bar\",345:\"foo\"}"],
		["[1u,{\"a\":1},2.30]", null],
		["<1:2>3", null],
		["[1,<7:8>9]", null],
		["<>1", "1"],
		["<8:3u>i{2:[[\".broker\",<1:2>true]]}", null],
		["<1:2,\"foo\":\"bar\">i{1:<7:8>9}", null],
		["<1:2,\"foo\":<5:6>\"bar\">[1u,{\"a\":1},2.30]", null],
		["i{1:2 // comment to end of line\n}", "i{1:2}"],
		["/*comment 1*/{ /*comment 2*/
		\t\"foo\"/*comment \"3\"*/: \"bar\", //comment to end of line
		\t\"baz\" : 1,
		/*
		\tmultiline comment
		\t\"baz\" : 1,
		\t\"baz\" : 1, // single inside multi
		*/
		}", "{\"baz\":1,\"foo\":\"bar\"}"],
		//["a[1,2,3]", "[1,2,3]"], // unsupported array type
		["<1:2>[3,<4:5>6]", null],
		["<4:\"svete\">i{2:<4:\"svete\">[0,1]}", null],
		[`d"2019-05-03T11:30:00-0700"`, `d"2019-05-03T11:30:00-07"`],
		//[`d""`, null],
		[`d"2018-02-02T00:00:00Z"`, null],
		[`d"2027-05-03T11:30:12.345+01"`, null],
		])
	{
		string cpon1 = lst[0];
		string cpon2 = lst[1]? lst[1]: cpon1;

		debug(chainpack) {log("cpon:", cpon1);}
		RpcValue rv1 = fromCpon(cpon1);
		debug(chainpack) {log("rv:", rv1);}
		auto cpk1 = rv1.toChainPack();
		debug(chainpack) {log("chainpack:", cpk1);}
		RpcValue rv2 = fromChainPack(cpk1);
		string cpn2 = rv2.toCpon();
		logD(cpon2, "\t--cpon------>\t", cpn2);
		assert(cpn2 == cpon2);
	}
}

void testDateTime()
{
	// same points in time
	RpcValue v1 = fromCpon(`d"2017-05-03T18:30:00Z"`);
	RpcValue v2 = fromCpon(`d"2017-05-03T22:30:00+04"`);
	RpcValue v3 = fromCpon(`d"2017-05-03T11:30:00-0700"`);
	RpcValue v4 = fromCpon(`d"2017-05-03T15:00:00-0330"`);
	assert(v1.datetime.msecsSinceEpoch == v2.datetime.msecsSinceEpoch);
	assert(v2.datetime.msecsSinceEpoch == v3.datetime.msecsSinceEpoch);
	assert(v3.datetime.msecsSinceEpoch == v4.datetime.msecsSinceEpoch);
	assert(v4.datetime.msecsSinceEpoch == v1.datetime.msecsSinceEpoch);
}

int main(string[] args)
{
	args = globalLog.setCLIOptions(args);
	test_vals();
	testConversions();
	testDateTime();
	logInfo("PASSED");
	return 0;
}
