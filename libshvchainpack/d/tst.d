import std.stdio;
import std.range;
import std.array;
import std.utf;
import std.file;
import std.algorithm;

void main()
{
	try
	{
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
	}
	catch (FileException ex)
	{
		writeln("errro: ", ex);
	}
}