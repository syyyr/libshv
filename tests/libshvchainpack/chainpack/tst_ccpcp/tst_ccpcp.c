#include <ccpcp.h>
#include <ccpon.h>
#include <cchainpack.h>
#include <ccpcp_convert.h>

#define _XOPEN_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

static void binary_dump(const uint8_t *buff, int len)
{
	for (size_t i = 0; i < len; ++i) {
		uint8_t u = buff[i];
		//ret += std::to_string(u);
		if(i > 0)
			printf("|");
		for (size_t j = 0; j < 8*sizeof(u); ++j) {
			printf((u & (((uint8_t)128) >> j))? "1": "0");
		}
	}
}

static inline char hex_nibble(char i)
{
	if(i < 10)
		return '0' + i;
	return 'A' + (i - 10);
}

static void hex_dump(const uint8_t *buff, int len)
{
	for (size_t i = 0; i < len; ++i) {
		char h = buff[i] / 16;
		char l = buff[i] % 16;
		printf("%c", hex_nibble(h));
		printf("%c", hex_nibble(l));
	}
}

int test_pack_double(double d, const char *res)
{
	static const unsigned long BUFFLEN = 1024;
	char buff[BUFFLEN];
	ccpcp_pack_context ctx;
	ccpcp_pack_context_init(&ctx, buff, BUFFLEN, NULL);
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
	ccpcp_pack_context ctx;
	ccpcp_pack_context_init(&ctx, buff, BUFFLEN, NULL);
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
	ccpcp_pack_context ctx;
	ccpcp_pack_context_init(&ctx, buff, BUFFLEN, NULL);
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
	static const size_t STATE_CNT = 100;
	ccpcp_container_state states[STATE_CNT];
	ccpcp_container_stack stack;
	ccpc_container_stack_init(&stack, states, STATE_CNT, NULL);
	unsigned long n = strlen(str);
	ccpcp_unpack_context ctx;
	ccpcp_unpack_context_init(&ctx, (uint8_t*)str, n, NULL, &stack);

	ccpon_unpack_next(&ctx);
	if(ctx.err_no != CCPCP_RC_OK) {
		printf("FAIL! unpack number str: '%s' error: %d\n", str, ctx.err_no);
		return -1;
	}
	if(ctx.item.type != expected_type) {
		printf("FAIL! unpack number str: '%s' have type: %d expected type: %d\n", str, ctx.item.type, expected_type);
		return -1;
	}
	switch(ctx.item.type) {
	case CCPCP_ITEM_DECIMAL: {
		double d = ctx.item.as.Decimal.mantisa;
		for (int i = 0; i < ctx.item.as.Decimal.dec_places; ++i)
			d /= 10;
		if(d == expected_val)
			return 0;
		printf("FAIL! unpack decimal number str: '%s' have: %lg expected: %lg\n", str, d, expected_val);
		return -1;
	}
	case CCPCP_ITEM_DOUBLE: {
		double d = ctx.item.as.Double;
		double epsilon = 1e-10;
		double diff = d - expected_val;
		if(diff > -epsilon && diff < epsilon)
			return 0;
		printf("FAIL! unpack double number str: '%s' have: %lg expected: %lg difference: %lg\n", str, d, expected_val, (d-expected_val));
		return -1;
	}
	case CCPCP_ITEM_INT: {
		int64_t d = ctx.item.as.Int;
		if(d == (int64_t)expected_val)
			return 0;
		printf("FAIL! unpack int number str: '%s' have: %ld expected: %ld\n", str, d, (int64_t)expected_val);
		return -1;
	}
	case CCPCP_ITEM_UINT: {
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
	static const size_t STATE_CNT = 100;
	ccpcp_container_state states[STATE_CNT];
	ccpcp_container_stack stack;
	ccpc_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context ctx;
	unsigned long n = strlen(str);
	ccpcp_unpack_context_init(&ctx, (uint8_t*)str, n, NULL, &stack);

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

	ccpon_unpack_next(&ctx);
	if(ctx.err_no != CCPCP_RC_OK) {
		printf("FAIL! unpack date time str: '%s' error: %d\n", str, ctx.err_no);
		return -1;
	}
	if(ctx.item.type != CCPCP_ITEM_DATE_TIME) {
		printf("FAIL! unpack number str: '%s' have type: %d expected type: %d\n", str, ctx.item.type, CCPCP_ITEM_DATE_TIME);
		return -1;
	}
	ccpcp_date_time *dt = &ctx.item.as.DateTime;
	if(dt->msecs_since_epoch == expected_epoch_msec && dt->minutes_from_utc == expected_utc_offset_min)
		return 0;
	printf("FAIL! unpack datetime str: '%s' have: %ld msec + %d utc min expected: %ld msec + %d utc min\n"
		   , str
		   , dt->msecs_since_epoch, dt->minutes_from_utc
		   , expected_epoch_msec, expected_utc_offset_min);
	return -1;
}

void test_cpon()
{
	const char* cpons[] = {
		//"1", NULL,
		"[]", NULL,
		"[1]", NULL,
		"[1,]", "[1]",
		"[1,2,3]", NULL,
		"[[]]", NULL,
	};
	size_t cpons_cnt = sizeof (cpons) / sizeof (char*) / 2;
	for(size_t i = 0; i < cpons_cnt; i += 2) {

		printf("-------------------------------------------------\n");

		static const size_t STATE_CNT = 100;
		ccpcp_container_state states[STATE_CNT];
		ccpcp_container_stack stack;

		const char *in_buff = cpons[i];
		ccpcp_unpack_context in_ctx;

		char out_buff2[1024];
		char out_buff1[1024];
		ccpcp_pack_context out_ctx;

		printf("Cpon: %s\n", in_buff);

		ccpc_container_stack_init(&stack, states, STATE_CNT, NULL);
		ccpcp_unpack_context_init(&in_ctx, in_buff, strlen(in_buff), NULL, &stack);
		ccpcp_pack_context_init(&out_ctx, out_buff1, sizeof (out_buff1), NULL);

		ccpcp_convert(&in_ctx, CCPCP_Cpon, &out_ctx, CCPCP_Cpon);
		*out_ctx.current = 0;
		printf("1. Cpon->Cpon: %s\n", out_ctx.start);
		//printf("DDD in err: %d out err: %d\n", in_ctx.err_no, out_ctx.err_no);
		assert(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);
		assert(!strcmp(in_buff, (const char *)out_ctx.start));

		ccpc_container_stack_init(&stack, states, STATE_CNT, NULL);
		ccpcp_unpack_context_init(&in_ctx, out_buff1, sizeof(out_buff1), NULL, &stack);
		ccpcp_pack_context_init(&out_ctx, out_buff2, sizeof (out_buff2), NULL);

		ccpcp_convert(&in_ctx, CCPCP_Cpon, &out_ctx, CCPCP_ChainPack);
		printf("2. Cpon->CPack len: %d data: ", out_ctx.current - out_ctx.start); binary_dump(out_ctx.start, out_ctx.current - out_ctx.start); printf("\n");
		assert(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);

		ccpc_container_stack_init(&stack, states, STATE_CNT, NULL);
		ccpcp_unpack_context_init(&in_ctx, out_buff2, sizeof(out_buff2), NULL, &stack);
		ccpcp_pack_context_init(&out_ctx, out_buff1, sizeof (out_buff1), NULL);

		ccpcp_convert(&in_ctx, CCPCP_ChainPack, &out_ctx, CCPCP_ChainPack);
		printf("3. CPack->CPack len: %d data: ", out_ctx.current - out_ctx.start); binary_dump(out_ctx.start, out_ctx.current - out_ctx.start); printf("\n");
		//printf("DDD %d %d\n", in_ctx.current - in_ctx.start, out_ctx.current - out_ctx.start);
		assert(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);
		assert(in_ctx.current - in_ctx.start == out_ctx.current - out_ctx.start);
		assert(!memcmp(in_ctx.start, out_ctx.start, out_ctx.current - out_ctx.start));

		ccpc_container_stack_init(&stack, states, STATE_CNT, NULL);
		ccpcp_unpack_context_init(&in_ctx, out_buff1, sizeof(out_buff1), NULL, &stack);
		ccpcp_pack_context_init(&out_ctx, out_buff2, sizeof (out_buff2), NULL);

		ccpcp_convert(&in_ctx, CCPCP_ChainPack, &out_ctx, CCPCP_Cpon);
		*out_ctx.current = 0;
		printf("4. CPack->Cpon: %s\n", out_ctx.start);
		assert(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);

		const char *ref_buff = cpons[i + 1];
		if(!ref_buff)
			ref_buff = in_buff;

		assert(!strcmp(in_buff, ref_buff));
	}
}

int main(int argc, const char * argv[])
{
	(void)argc;
	(void)argv;

	for (int i = CP_Null; i <= CP_CString; ++i) {
		printf("%d %x ", i, i);
		binary_dump((uint8_t*)&i, 1);
		printf(" %s\n", cchainpack_packing_schema_name(i));
	}
	for (int j = CP_Null; j <= CP_CString; ++j) {
		uint8_t i = j | CP_ARRAY_FLAG_MASK;
		printf("%d %x ", i, i);
		binary_dump((uint8_t*)&i, 1);
		printf(" %s_Array\n", cchainpack_packing_schema_name(j));
	}
	for (int i = CP_FALSE; i <= CP_TERM; ++i) {
		printf("%d %x ", i, i);
		binary_dump((uint8_t*)&i, 1);
		printf(" %s\n", cchainpack_packing_schema_name(i));
	}

	printf("C Cpon test started.\n");

	test_pack_int(1, "1");
	test_pack_int(-1234567890l, "-1234567890");

	test_pack_uint(1, "1u");
	test_pack_uint(1234567890l, "1234567890u");

	test_pack_double(0.1, "0.1");
	test_pack_double(1, "1.");
	test_pack_double(1.2, "1.2");

	test_unpack_number("1", CCPCP_ITEM_INT, 1);
	test_unpack_number("123u", CCPCP_ITEM_UINT, 123);
	test_unpack_number("+123", CCPCP_ITEM_INT, 123);
	test_unpack_number("-1", CCPCP_ITEM_INT, -1);
	test_unpack_number("1u", CCPCP_ITEM_UINT, 1);
	test_unpack_number("0.1", CCPCP_ITEM_DOUBLE, 0.1);
	test_unpack_number("1.", CCPCP_ITEM_DOUBLE, 1);
	test_unpack_number("1e2", CCPCP_ITEM_DOUBLE, 100);
	test_unpack_number("1.e2", CCPCP_ITEM_DOUBLE, 100);
	test_unpack_number("1.23", CCPCP_ITEM_DOUBLE, 1.23);
	test_unpack_number("1.23e4", CCPCP_ITEM_DOUBLE, 1.23e4);
	test_unpack_number("-21.23e-4", CCPCP_ITEM_DOUBLE, -21.23e-4);
	test_unpack_number("-0.567e-3", CCPCP_ITEM_DOUBLE, -0.567e-3);
	test_unpack_number("1.23n", CCPCP_ITEM_DECIMAL, 1.23);

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

	test_cpon();
}
