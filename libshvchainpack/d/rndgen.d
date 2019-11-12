import std.stdio;
import std.conv;

ulong gcd(ulong a, ulong b)
{
	while (b) {
		auto t = b;
		b = a % b;
		a = t;
	}
	return a;
}

unittest {
	assert(primeFactorsOnly(100) == 10);
	assert(primeFactorsOnly(11) == 11);
	assert(primeFactorsOnly(32) == 2);
	assert(primeFactorsOnly(7*7*11*11*15) == 7*11*15);
	assert(primeFactorsOnly(129*2) == 129*2);
}

ulong primeFactorsOnly(ulong n)
{
	ulong accum = 1;
	ulong iter = 2;
	for ( ; n >= iter; iter += 2 - (iter == 2? 1: 0)) {
		if (n % iter)
			continue;
		accum *= iter;
		do {
			n /= iter;
		} while (n % iter == 0);
	}
	return accum * n;
}

unittest {
	// broken example
	assert(!properLinearCongruentialParameters(2147483645UL, 16820, 9));
	// Numerical recipes book
	assert(properLinearCongruentialParameters(1UL << 32, 1664525, 1013904223));
	// Borland C/C++ compiler
	assert(properLinearCongruentialParameters(1UL << 32, 22695477, 1));
	// glibc
	assert(properLinearCongruentialParameters(1UL << 32, 1103515245, 12345));
	// ANSI C
	assert(properLinearCongruentialParameters(1UL << 32, 134775813, 1));
	// Microsoft Visual C/C++
	assert(properLinearCongruentialParameters(1UL << 32, 214013, 2531011));
}

bool properLinearCongruentialParameters(ulong n, ulong a, ulong c)
{
    // Bounds checking
    if (n == 0 || a == 0 || a >= n || c == 0 || c >= n)
    	return false;
    // c and n are relatively prime
    if (gcd(c, n) != 1)
    	return false;
    // a - 1 is divisible by all prime factors of n
    if ((a - 1) % primeFactorsOnly(n))
    	return false;
    // If a - 1 is multiple of 4, then n is a multiple of 4, too.
    if ((a - 1) % 4 == 0 && n % 4)
    	return false;
    // Passed all tests
    return true;
}

auto makeRndGen(ulong n, ulong a, ulong c)
{
	if(!properLinearCongruentialParameters(n, a, c))
		throw new Exception("Weak generator params");
	return (ulong x) { return x * a + c; };
}

void main(string[] args)
{
	if(args.length != 4)
		throw new Exception("There must be N a c parameters specified in command line, x = x * a + c // N");
	//foreach(arg; args[1..$])
	//	writeln(arg);
    //auto gen = makeRndGen(2147483645, 16820, 9);
    ulong n = to!ulong(args[1]);
    ulong a = to!ulong(args[2]);
    ulong c = to!ulong(args[3]);
    //auto gen = makeRndGen(1UL << 32, 1234567, 13);
    auto gen = makeRndGen(n, a, c);
    ulong seed = 1;
    foreach(i; 1..100) {
    	seed = gen(seed);
    	writeln((seed % 9999) + 1);
    }
}
