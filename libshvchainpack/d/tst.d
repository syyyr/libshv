import std.stdio;
import std.traits;

struct A
{
	int x = 1;

	this(T : A)(inout T arg) inout
	{
		x = arg.x;
		writeln("ctor: ", x);
	}
	void opAssign(T)(T arg) if (!isStaticArray!T && !is(T : A))
	{
		x = 123;
		writeln("assign: ", x);
	}

	void opAssign(T)(ref T arg) if (isStaticArray!T)
	{
		x = 321;
		writeln("assign: ", x);
	}
	void opAssign(A arg)
	{
		x = 567;
		writeln("assign: ", x);
	}
}

struct B
{
	int x = 1;

	this(int x)
	{
		this.x = x;
		writeln("B int ctor: ", x);
	}
	this(ref return scope const B b)
	{
		x = b.x;
		writeln("B copy ctor: ", x);
	}
	//this(this) {writeln("B postblit: ", x); }
	void opAssign(ref return scope const B b)
	{
		x = b.x;
		writeln("B assign: ", x);
	}
}

struct S
{
	ref inout(A) opIndex(int key) inout @safe { return values[key]; }
	void opIndexAssign(T)(auto ref T val, int key) { values[key] = val; }
	private A[int] values;
}

A foo(const ref S s, int ix)
{
	A a2 = s[ix];
	a2.x++;
	return a2;
}

struct M
{
	public this(int[int] vals) {m_values = vals;}
	public auto byKey() const @safe { return m_values.byKey();}
	public ref inout(int) opIndex(int key) inout @safe { return m_values[key]; }
	public void print() const
	{
		printM(this);
	}
	private int[int] m_values;
}

void printM(const ref M m)
{
	writeln("M -> ", typeid(m));
	foreach(k; m.byKey()) {
		writeln(k, " -> ", m[k]);
	}
}

void testM()
{
	M m = [1:2, 3:4,];
	printM(m);
	m.print();
}

void main()
{
	//S s;
	//s[1] = A();
	//writeln("s[1]: ", s[1].x);
	//A a = foo(s, 1);
	//writeln("a: ", a);
	//const(A) a1 = a;
	//A a2 = a1;
	//writeln("a2: ", a2);

	//B b = 1;
	//B b2 = b;
	//b = b2;
	//const(B) b1 = 2;
	//b = b1;
	//writeln("b2: ", b2);
	testM();
}
