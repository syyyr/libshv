"use strict"

class Test
{
	static checkEq(e1, e2, msg)
	{
		//console.log((e1 === e2)? "OK": "ERROR", ":", e1, "vs.", e2)
		if(e1 === e2)
			return;
		if(msg)
			throw msg;
		else
			throw "test check error: " + e1 + " === " + e2
	}

	static run()
	{
		try {
			for(const lst of [
				["true", null],
				["false", null],
				["null", null],
				["1u", null],
				["134", null],
				["223.", null],
				["2.30", null],
				["12.3e-10", "123e-11"],
				["-0.00012", "-12e-5"],
				["-1234567890.", "-1234567890."],
				["[]", null],
				["[1]", null],
				["[1,]", "[1]"],
				["[1,2,3]", null],
				["[[]]", null],
				["{\"foo\":\"bar\"}", null],
				["i{1:2}", null],
				["i{\n\t1: \"bar\",\n\t345u : \"foo\",\n}", "i{1:\"bar\",345:\"foo\"}"],
				["[1u,{\"a\":1},2.30]", null],
				["<>1", null],
				["<1:2>3", null],
				["[1,<7:8>9]", null],
				["<8:3u>i{2:[[\".broker\",<1:2>true]]}", null],
				["<1:2,\"foo\":\"bar\">i{1:<7:8>9}", null],
				["<1:2,\"foo\":<5:6>\"bar\">[1u,{\"a\":1},2.30]", null],
				["i{1:2 // comment to end of line\n}", "i{1:2}"],
				[`/*comment 1*/{ /*comment 2*/
				\t\"foo\"/*comment \"3\"*/: \"bar\", //comment to end of line
				\t\"baz\" : 1,
				/*
				\tmultiline comment
				\t\"baz\" : 1,
				\t\"baz\" : 1, // single inside multi
				*/
				}`, "{\"foo\":\"bar\",\"baz\":1}"],
				//["a[1,2,3]", "[1,2,3]"], // unsupported array type
				["<1:2>[3,<4:5>6]", null],
				["<4:\"svete\">i{2:<4:\"svete\">[0,1]}", null],
				['d""', null],
				['d"2017-05-03T11:30:00-0700"', 'd"2017-05-03T11:30:00-07"'],
				['d"2017-05-03T11:30:12.345+01"', null],
				]) {
				let cpon1 = lst[0]
				//console.log(cpon1)
				let rpc_val = RpcValue.fromCpon(cpon1);
				let cpon2 = rpc_val.toString();
				console.log(cpon1, "vs.", cpon2)
				if(lst[1])
					cpon1 = lst[1]
				Test.checkEq(cpon1, cpon2);
				{
					// same points in time
					let v1 = RpcValue.fromCpon('d"2017-05-03T18:30:00Z"');
					let v2 = RpcValue.fromCpon('d"2017-05-03T22:30:00+04"');
					let v3 = RpcValue.fromCpon('d"2017-05-03T11:30:00-0700"');
					let v4 = RpcValue.fromCpon('d"2017-05-03T15:00:00-0330"');
					Test.checkEq(v1.value.epochMsec, v2.value.epochMsec);
					Test.checkEq(v2.value.epochMsec, v3.value.epochMsec);
					Test.checkEq(v3.value.epochMsec, v4.value.epochMsec);
					Test.checkEq(v4.value.epochMsec, v1.value.epochMsec);

				}
			}
			console.log("PASSED")
		}
		catch(err) {
			console.log("FAILED:", err)
		}
	}
}
