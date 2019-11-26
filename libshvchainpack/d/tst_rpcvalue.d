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
			ulong n = (cast(ulong)1) << (i*8 + j*3+1);
			auto rv = RpcValue(n);
			auto cpon = rv.toCpon();
			auto cpk = rv.toChainPack();
			auto rv_cpon = RpcValue.fromCpon(cpon);
			auto rv_cpk = RpcValue.fromChainPack(cpk);
			logD(n, cpon, cpk);
			{
				import std.stdio : writeln;
				writeln(cpk);
				writeln(cpk.length, rv_cpon.uinteger, rv_cpk);
			}
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
				long n = sig * (cast(long)1) << (i*8 + j*2+2);
				auto rv = RpcValue(n);
				auto cpon = rv.toCpon();
				auto cpk = rv.toChainPack();
				auto rv_cpon = RpcValue.fromCpon(cpon);
				auto rv_cpk = RpcValue.fromChainPack(cpk);
				logD(n, cpon, cpk);
				assert(to!string(n) == cpon);
				assert(rv_cpon == rv_cpk);
				assert(rv_cpk.integer == n);
			}
		}
	}
	/+
	for (int i = 0; i < 2; i++) {
		long n = 1;
		n <<= sizeof (n) * 8 - 1; // -MIN INT
		if(i)
			n = ~n; // MAX INT
		else
			n = n + 1; // MIN INT + 1
		INIT_BUFFS();
		ccpon_pack_int(&out_ctx, n);
		*out_ctx.current = 0;
		logsn(out_buff2, sizeof (out_buff2), "%lli", (long long)n);
		test_cpon((const char *)out_ctx.start, out_buff2);
	}
	log("------------- decimal ");
	{
		int mant = -123456;
		int exp_min = 1;
		int exp_max = 16;
		int step = 1;
		for (int exp = exp_min; exp <= exp_max; exp += step) {
			INIT_BUFF();
			ccpon_pack_decimal(&out_ctx, mant, exp);
			*out_ctx.current = 0;
			test_cpon((const char *)out_ctx.start, NULL);
		}
	}
	{
		log("------------- double");
		{
			double n_max = 1000000.;
			double n_min = -1000000.;
			double step = (n_max - n_min) / 100.1;
			for (double n = n_min; n < n_max; n += step) {
				INIT_BUFF();
				cchainpack_pack_double(&out_ctx, n);
				assert(out_ctx.current - out_ctx.start == sizeof(double) + 1);
				ccpcp_unpack_context in_ctx;
				ccpcp_unpack_context_init(&in_ctx, out_buff1, sizeof(out_buff1), NULL, NULL);
				cchainpack_unpack_next(&in_ctx);
				assert(in_ctx.current - in_ctx.start == sizeof(double) + 1);
				assert(n == in_ctx.item.as.Double);
			}
		}
		{
			double n_max = DBL_MAX;
			double n_min = DBL_MIN;
			double step = -1.23456789e10;
			//qDebug() << n_min << " - " << n_max << ": " << step << " === " << (n_max / step / 10);
			for (double n = n_min; n < n_max / -step / 10; n *= step) {
				INIT_BUFF();
				log("%g\n", n);
				cchainpack_pack_double(&out_ctx, n);
				ccpcp_unpack_context in_ctx;
				ccpcp_unpack_context_init(&in_ctx, out_buff1, sizeof(out_buff1), NULL, NULL);
				cchainpack_unpack_next(&in_ctx);
				//if(n > -100 && n < 100)
				//	qDebug() << n << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << len << " dump: " << binary_dump(out.str()).c_str();
				assert(in_ctx.current - in_ctx.start == sizeof(double) + 1);
				assert(n == in_ctx.item.as.Double);
			}
		}
	}
	log("------------- datetime ");
	{
		const char *cpons[] = {
			//"d\"\"", NULL,
			"d\"2018-02-02T0:00:00.001\"", "d\"2018-02-02T00:00:00.001Z\"",
			"d\"2018-02-02 01:00:00.001+01\"", "d\"2018-02-02T01:00:00.001+01\"",
			"d\"2018-12-02 0:00:00\"", "d\"2018-12-02T00:00:00Z\"",
			"d\"2041-03-04 0:00:00-1015\"", "d\"2041-03-04T00:00:00-1015\"",
			"d\"2041-03-04T0:00:00.123-1015\"", "d\"2041-03-04T00:00:00.123-1015\"",
			"d\"1970-01-01T0:00:00\"", "d\"1970-01-01T00:00:00Z\"",
			"d\"2017-05-03T5:52:03\"", "d\"2017-05-03T05:52:03Z\"",
			"d\"2017-05-03T15:52:03.923Z\"", NULL,
			"d\"2017-05-03T15:52:03.000-0130\"", "d\"2017-05-03T15:52:03-0130\"",
			"d\"2017-05-03T15:52:03.923+00\"", "d\"2017-05-03T15:52:03.923Z\"",
		};
		for (size_t i = 0; i < sizeof (cpons) / sizeof(char*); i+=2) {
			const char *cpon = cpons[i];
			INIT_BUFF();
			test_cpon(cpon, cpons[i+1]);
		}
	}
	log("------------- cstring ");
	{
		const char *cpons[] = {
			"", NULL,
			"hello", NULL,
			"\t\r", "\"\\t\\r\"",
			"\\0", "\"\\\\0\"",
			"1\t\r\n\b", "\"1\\t\\r\\n\\b\"",
			"escaped zero \\0 here \t\r\n\b", "\"escaped zero \\\\0 here \\t\\r\\n\\b\"",
		};
		for (size_t i = 0; i < sizeof (cpons) / sizeof(char*); i+=2) {
			const char *cpon = cpons[i];
			INIT_BUFF();
			ccpon_pack_string_terminated(&out_ctx, cpon);
			*out_ctx.current = 0;
			test_cpon2(out_ctx.start, cpons[i+1]);
		}
	}
	{
		INIT_BUFF();
		const char str[] = "zero \0 here";
		ccpon_pack_string_start(&out_ctx, str, sizeof(str)-1);
		ccpon_pack_string_cont(&out_ctx, "ahoj", 4);
		ccpon_pack_string_finish(&out_ctx);
		*out_ctx.current = 0;
		test_cpon(out_ctx.start, "\"zero \\0 here""ahoj\"");
	}
	+/
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
		RpcValue rv1 = RpcValue.fromCpon(cpon1);
		debug(chainpack) {log("rv:", rv1);}
		auto cpk1 = rv1.toChainPack();
		debug(chainpack) {log("chainpack:", cpk1);}
		RpcValue rv2 = RpcValue.fromChainPack(cpk1);
		string cpn2 = rv2.toCpon();
		logD(cpon2, "\t--cpon------>\t", cpn2);
		assert(cpn2 == cpon2);
	}
}

@safe
void testDateTime()
{
	// same points in time
	RpcValue v1 = RpcValue.fromCpon(`d"2017-05-03T18:30:00Z"`);
	RpcValue v2 = RpcValue.fromCpon(`d"2017-05-03T22:30:00+04"`);
	RpcValue v3 = RpcValue.fromCpon(`d"2017-05-03T11:30:00-0700"`);
	RpcValue v4 = RpcValue.fromCpon(`d"2017-05-03T15:00:00-0330"`);
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
