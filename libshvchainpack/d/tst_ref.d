int* ptr_ref(ref int x) @trusted
{
	return &x;
}

void main() @safe
{
	int a = 1;
	//int *b = &a; // Error: variable `tst_ref.main.b` only parameters or `foreach` declarations can be `ref`
	ref int b() { return a; }
	b = 2;
	assert(a == 2);
	auto p = ptr_ref(a);
	*p = 3;
	assert(a == 3);
}
