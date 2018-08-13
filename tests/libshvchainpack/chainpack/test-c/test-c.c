#include <ccpon.h>

#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>

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

int test_pack_int(long i, const char *res)
{
	static const unsigned long BUFFLEN = 1024;
	char buff[BUFFLEN];
	ccpon_pack_context ctx;
	ccpon_pack_context_init(&ctx, buff, BUFFLEN, NULL);
	ccpon_pack_int(&ctx, i);
	*ctx.current = '\0';
	if(strcmp(buff, res)) {
		printf("FAIL! pack signed %ld have: '%s' expected: '%s'\n", i, buff, res);
		return -1;
	}
	return 0;
}

int test_pack_uint(unsigned long i, const char *res)
{
	static const unsigned long BUFFLEN = 1024;
	char buff[BUFFLEN];
	ccpon_pack_context ctx;
	ccpon_pack_context_init(&ctx, buff, BUFFLEN, NULL);
	ccpon_pack_uint(&ctx, i);
	*ctx.current = '\0';
	if(strcmp(buff, res)) {
		printf("FAIL! pack unsigned %lu have: '%s' expected: '%s'\n", i, buff, res);
		return -1;
	}
	return 0;
}

int test_unpack_number(const char *str, int expected_type, double expected_val)
{
	ccpon_unpack_context ctx;
	unsigned long n = strlen(str);
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
		double epsilon = 1e-10;
		double diff = d - expected_val;
		if(diff > -epsilon && diff < epsilon)
			return 0;
		printf("FAIL! unpack double number str: '%s' have: %lg expected: %lg difference: %lg\n", str, d, expected_val, (d-expected_val));
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

int test_unpack_datetime(const char *str, int add_msecs, int expected_utc_offset_min)
{
	unsigned long n = strlen(str);
	struct tm tm;
	int has_T = 0;
	for (unsigned long i = 0; i < n; ++i) {
		if(str[i] == 'T') {
			has_T = 1;
			break;
		}
	}
	const char *dt_format = has_T? "%Y-%m-%dT%H:%M:%S": "%Y-%m-%d %H:%M:%S";
	char *rest = strptime(str+2, dt_format, &tm);
	//printf("\tstr: '%s' year: %d month: %d day: %d rest: '%s'\n", str , tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday , rest);
	int64_t expected_epoch_msec = timegm(&tm);
	expected_epoch_msec *= 1000;
	expected_epoch_msec += add_msecs;
	ccpon_unpack_context ctx;
	ccpon_unpack_context_init(&ctx, str, n, NULL);
	ccpon_unpack_next(&ctx);
	if(ctx.err_no != CCPON_RC_OK) {
		printf("FAIL! unpack date time str: '%s' error: %d\n", str, ctx.err_no);
		return -1;
	}
	if(ctx.item.type != CCPON_ITEM_DATE_TIME) {
		printf("FAIL! unpack number str: '%s' have type: %d expected type: %d\n", str, ctx.item.type, CCPON_ITEM_DATE_TIME);
		return -1;
	}
	ccpon_date_time *dt = &ctx.item.as.DateTime;
	if(dt->msecs_since_epoch == expected_epoch_msec && dt->minutes_from_utc == expected_utc_offset_min)
		return 0;
	printf("FAIL! unpack datetime str: '%s' have: %ld msec + %d utc min expected: %ld msec + %d utc min\n"
		   , str
		   , dt->msecs_since_epoch, dt->minutes_from_utc
		   , expected_epoch_msec, expected_utc_offset_min);
	return -1;
}

int main(int argc, const char * argv[])
{
	(void)argc;
	(void)argv;
	printf("C Cpon test started.\n");

	test_pack_int(1, "1");
	test_pack_int(-1234567890l, "-1234567890");

	test_pack_uint(1, "1u");
	test_pack_uint(1234567890l, "1234567890u");

	test_pack_double(0.1, "0.1");
	test_pack_double(1, "1.");
	test_pack_double(1.2, "1.2");

	test_unpack_number("1", CCPON_ITEM_INT, 1);
	test_unpack_number("123u", CCPON_ITEM_UINT, 123);
	test_unpack_number("+123", CCPON_ITEM_INT, 123);
	test_unpack_number("-1", CCPON_ITEM_INT, -1);
	test_unpack_number("1u", CCPON_ITEM_UINT, 1);
	test_unpack_number("0.1", CCPON_ITEM_DOUBLE, 0.1);
	test_unpack_number("1.", CCPON_ITEM_DOUBLE, 1);
	test_unpack_number("1e2", CCPON_ITEM_DOUBLE, 100);
	test_unpack_number("1.e2", CCPON_ITEM_DOUBLE, 100);
	test_unpack_number("1.23", CCPON_ITEM_DOUBLE, 1.23);
	test_unpack_number("1.23e4", CCPON_ITEM_DOUBLE, 1.23e4);
	test_unpack_number("-21.23e-4", CCPON_ITEM_DOUBLE, -21.23e-4);
	test_unpack_number("-0.567e-3", CCPON_ITEM_DOUBLE, -0.567e-3);
	test_unpack_number("1.23n", CCPON_ITEM_DECIMAL, 1.23);

	test_unpack_datetime("d\"2018-02-02T0:00:00.001\"", 1, 0);
	test_unpack_datetime("d\"2018-02-02 01:00:00.001+01\"", 1, 60);
	test_unpack_datetime("d\"2018-12-02 0:00:00\"", 0, 0);
	test_unpack_datetime("d\"2041-03-04 0:00:00-1015\"", 0, -(10*60+15));
	test_unpack_datetime("d\"2041-03-04 0:00:00.123-1015\"", 123, -(10*60+15));
	test_unpack_datetime("d\"1970-01-01 0:00:00\"", 0, 0);
	test_unpack_datetime("d\"2017-05-03 5:52:03\"", 0, 0);
	test_unpack_datetime("d\"2017-05-03T15:52:03.923Z\"", 923, 0);
	test_unpack_datetime("d\"2017-05-03T15:52:03.000-0130\"", 0, -(1*60+30));
	test_unpack_datetime("d\"2017-05-03T15:52:03.923+00\"", 923, 0);
}
