import shv.cpon;
//import shv.chainpack;
import shv.rpcvalue;
import std.array;

void main()
{
	foreach(i; 0 .. 1000) {
		RpcValue v = RpcValue.DateTime.now();
		string cpon = write(v);
		debug {
			import std.stdio : writeln;
			writeln(cpon);
		}
	}
}