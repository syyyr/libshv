import shv.rpcvalue;
import shv.cpon;
import shv.log;

import std.stdio;
import std.algorithm;
import std.typecons;
import std.conv;

alias log = logInfo;

void testConversions()
{
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

		//log("src:", cpon1);
		RpcValue rv1 = RpcValue.fromCpon(cpon1);
		//log(cpon1, rv1.type, rv1.integer);
		string cpn1 = rv1.toCpon();
		log(cpon1, "\t--cpon------>\t", cpn1);
		assert(cpn1 == cpon2);
		/*
		let cpk1 = rv1.toChainPack();
		let rv2 = RpcValue.fromChainPack(cpk1);
		let cpn2 = rv2.toString();
		log(cpn1, "\t--chainpack->\t", cpn2, "\n")
		this.checkEq(cpn1, cpn2);
		*/
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

int main()
{
	debug {
		auto writemeta = (string s, Meta m) {
			writeln("Meta ", s, ":");
			foreach(k, v; m)
				writeln('\t', k, " ---> ", v);
		};

		Meta mt1;
		mt1["foo"] = "bar";
		mt1[1] = 42;
		writemeta("mt1", mt1);

		auto mt2 = mt1;
		mt2["foo"] = "baz";
		writemeta("mt2", mt2);
		writemeta("mt1", mt1);
		writeln("mt1 ", mt1);

		assert(mt1["foo"] == mt2["foo"]);

		string[string] m = ["ahoj": "babi", "bar": "baz"];
		RpcValue v = m;
		v["foo"] = cast(RpcValue)[1,2,3,4,5,];
		v.meta[1] = 42;
		RpcValue v2 = v["foo"];
		v2.list ~= RpcValue(["99":99, "sto":100]);
		v["v2"] = v2;
		v.meta["foo"] = [1,2,3,4];

		writeln("rpcval: ", v);
		//writemeta("rpcval.meta", v.meta);
		writeln(v.toCpon());
		writeln(v.toCpon(Yes.pretty));
	}
	testConversions();
	testDateTime();
	return 0;
}
