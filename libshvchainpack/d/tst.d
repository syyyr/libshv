import std.stdio;
import std.range;
import std.array;
import std.utf;
import std.file;
import std.algorithm;

import shv.log;

struct Reader(R)
{
	enum short NO_BYTE = -1;

	int count = 0;

	short delegate() @safe peek_byte;
	ubyte delegate() @safe get_byte;

	this(ref R input_range) @safe
	{
		short peek_byte() @safe
		{
			if(input_range.empty())
				return NO_BYTE;
			return input_range.front();
		}

		ubyte get_byte() @safe
		{
			ubyte ret = input_range.front();
			input_range.popFront();
			count++;
			return ret;
		}

		this.peek_byte = &peek_byte;
		this.get_byte = &get_byte;
	}
}

void main()
{
	ubyte[] data = [1,2,3,4,5,6,7,8,9];
	auto reader = Reader!(ubyte[])(data);
	while (reader.peek_byte() != reader.NO_BYTE) {
		auto b = reader.get_byte();
		logInfo("byte:", b, reader.peek_byte(), reader.NO_BYTE, (reader.peek_byte() != reader.NO_BYTE));
		//logInfo("data:", data);
	}
}