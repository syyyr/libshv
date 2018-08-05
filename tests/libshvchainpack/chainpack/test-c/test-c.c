#include <ccpon.h>

#include <stdio.h>
#include <string.h>

int test_pack_double(double d, const char *res)
{
	static const unsigned long BUFFLEN = 1024;
	char buff[BUFFLEN];
	ccpon_pack_context ctx;
	ccpon_pack_context_init(&ctx, buff, BUFFLEN, NULL);
	ccpon_pack_double(&ctx, d);
	*ctx.current = '\0';
	if(strcmp(buff, res)) {
		printf("FAIL! pack double %lg have: '%s' expected: '%s'\n", d, buff, res);
		return -1;
	}
	return 0;
}

int test_unpack_number(const char *str, unsigned long n, int expected_type, double expected_val)
{
	ccpon_unpack_context ctx;
	ccpon_unpack_context_init(&ctx, str, n, NULL);
	ccpon_unpack_next(&ctx);
	if(ctx.err_no != CCPON_RC_OK) {
		printf("FAIL! unpack number str: '%s' error: %d\n", str, ctx.err_no);
		return -1;
	}
	if(ctx.item.type != expected_type) {
		printf("FAIL! unpack number str: '%s' have type: %d expected type: %d\n", str, ctx.item.type, expected_type);
		return -1;
	}
	switch(ctx.item.type) {
	case CCPON_ITEM_DECIMAL: {
		double d = ctx.item.as.Decimal.mantisa;
		for (int i = 0; i < ctx.item.as.Decimal.dec_places; ++i)
			d /= 10;
		if(d == expected_val)
			return 0;
		printf("FAIL! unpack decimal number str: '%s' have: %lg expected: %lg\n", str, d, expected_val);
		return -1;
	}
	case CCPON_ITEM_DOUBLE: {
		double d = ctx.item.as.Double;
		if(d == expected_val)
			return 0;
		printf("FAIL! unpack double number str: '%s' have: %lg expected: %lg\n", str, d, expected_val);
		return -1;
	}
	case CCPON_ITEM_INT: {
		int64_t d = ctx.item.as.Int;
		if(d == (int64_t)expected_val)
			return 0;
		printf("FAIL! unpack int number str: '%s' have: %ld expected: %ld\n", str, d, (int64_t)expected_val);
		return -1;
	}
	case CCPON_ITEM_UINT: {
		uint64_t d = ctx.item.as.UInt;
		if(d == (uint64_t)expected_val)
			return 0;
		printf("FAIL! unpack int number str: '%s' have: %lu expected: %lu\n", str, d, (uint64_t)expected_val);
		return -1;
	}
	default:
		printf("FAIL! unpack number str: '%s' unexpected type: %d\n", str, ctx.item.type);
		return -1;
	}
}

int main(int argc, const char * argv[])
{
	(void)argc;
	(void)argv;
	printf("C Cpon test started.\n");

	test_pack_double(1, "1.");
	test_pack_double(1.2, "1.2");

	test_unpack_number("1", 1, CCPON_ITEM_INT, 1);
	test_unpack_number("+123", 4, CCPON_ITEM_INT, 123);
	test_unpack_number("-1", 2, CCPON_ITEM_INT, -1);
	test_unpack_number("1u", 2, CCPON_ITEM_UINT, 1);
	test_unpack_number("0.1", 3, CCPON_ITEM_DOUBLE, 0.1);
	test_unpack_number("1.", 2, CCPON_ITEM_DOUBLE, 1);
	test_unpack_number("1e2", 3, CCPON_ITEM_DOUBLE, 100);
	test_unpack_number("1.e2", 4, CCPON_ITEM_DOUBLE, 100);
	test_unpack_number("1.23", 4, CCPON_ITEM_DOUBLE, 1.23);
	test_unpack_number("1.23e4", 6, CCPON_ITEM_DOUBLE, 1.23e4);
	test_unpack_number("1.23n", 5, CCPON_ITEM_DECIMAL, 1.23);
}
