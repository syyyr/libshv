import std.stdio;
import std.range;
import std.array;
import std.utf;
import std.file;
import std.algorithm;

mixin template RT()
{
	struct Reader
	{
		short peek_byte() @safe
		{
			if(input_range.empty())
				return -1;
			return input_range.front();
		}

		ubyte get_byte() @safe
		{
			ubyte ret = input_range.front();
			input_range.popFront();
			//pos++;
			return ret;
		}
	}
}

@safe get_n(Reader)(Reader reader, int n)
{
	for(int i=0; i<n; i++)
		reader.get_byte();
}

@safe range_test(R)(ref R input_range)
{
	mixin RT;
	get_n(Reader(), 2);
}

void main()
{
	ubyte[] data = [1,2,3,4,5,6,7,8,9];
	range_test(data);
	int n = 1;
	writeln(n, " not: ", ~n);
	ubyte b = 1;
	writeln(b, " not b: ", cast(ubyte)~cast(int)b);
	writeln(double.sizeof);

	try
	{
		/+
		std.file.write("test.txt", "ščřžýáí");
		auto f = File("test.txt", "r");
		/*
		foreach (ubyte[] s; f.byChunk(4)) {
			int n;
			while(!s.empty) {
				auto c = s.front();
				if(n++ == 0) {
					writeln(typeid(c));
					writeln(s.length);
				}

				writeln(n, ": ", c);
				s.popFront();
			}
		}
		*/
		{
			auto s = f.byChunk(10).joiner;
			int n;
			while(!s.empty) {
				size_t cnt;
				auto c = s.decodeFront(cnt);
				if(n++ == 0) {
					writeln(typeid(c));
					//writeln(s.length);
				}

				writeln(n, " cnt: ", cnt, " : ", c);
				//s.popFront();
			}
		}
		+/
	}
	catch (Exception ex)
	{
		writeln("errro: ", ex);
	}
}