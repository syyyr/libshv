//import std.stdio;
//import std.range;
import std.array;
//import std.utf;
//import std.file;
//import std.algorithm;

//import shv.log;

enum ImplType {Mixin, Struct, Inlined, Pointer}
enum impl_type = ImplType.Inlined;

static if(impl_type == ImplType.Mixin) {
	pragma(msg, "Mixin");
	mixin template GenReader(R)
	{
		struct Reader
		{
			enum short NO_BYTE = -1;

			int count = 0;

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
		}
	}
}
else static if (impl_type == ImplType.Struct) {
	pragma(msg, "Struct");
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
}
else static if (impl_type == ImplType.Inlined) {
	pragma(msg, "Inlined");
}
else static if (impl_type == ImplType.Pointer) {
	pragma(msg, "Pointer");
	struct Reader(R)
	{
		enum short NO_BYTE = -1;

		int count = 0;
		R *prange;

		this(ref R input_range) @trusted
		{
			prange = &input_range;
		}
		short peek_byte() @safe
		{
			if(prange.empty())
				return NO_BYTE;
			return (*prange).front();
		}

		ubyte get_byte() @safe
		{
			ubyte ret = (*prange).front();
			(*prange).popFront();
			count++;
			return ret;
		}
	}
}

void read(R)(ref R data)
{
	static if(impl_type == ImplType.Mixin) {
		alias input_range = data;
		mixin GenReader!R;
		auto reader = Reader();
	}
	else static if (impl_type == ImplType.Struct) {
		auto reader = Reader!R(data);
	}
	else static if (impl_type == ImplType.Inlined) {
		alias input_range = data;
		struct Reader
		{
			enum short NO_BYTE = -1;

			int count = 0;

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
		}
		auto reader = Reader();
	}
	else static if (impl_type == ImplType.Pointer) {
		auto reader = Reader!R(data);
	}
	while (reader.peek_byte() != reader.NO_BYTE) {
		auto b1 = reader.peek_byte();
		auto b = reader.get_byte();
		assert(b1 == b);
		//logInfo("byte:", b, reader.peek_byte(), reader.NO_BYTE, (reader.peek_byte() != reader.NO_BYTE));
		//logInfo("data:", data);
	}
}

void main()
{
	ubyte[] data = [1,2,3,4,5,6,7,8,9];
	read(data);
}