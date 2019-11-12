import shv.rpcvalue;
import shv.cpon;
import std.stdio;
import std.algorithm;
import std.typecons;

int main()
{
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
	writeln(v.toString(Yes.pretty));
	return 0;
}
