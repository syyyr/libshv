#include "../../libshvchainpack/c/ccpon.h"
#include "../../libshvchainpack/c/ccpcp.h"
#include "../../libshvchainpack/c/cchainpack.h"

#include <node_api.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
#undef NULL
#define NULL nullptr
#endif

typedef struct cpjs_env
{
	napi_env env;
	ccpcp_unpack_context *inCtx;
} cpjs_env;

static void cpjs_env_init(cpjs_env *self, napi_env env, ccpcp_unpack_context *ctx)
{
	self->env = env;
	self->inCtx = ctx;
}

static void logs(napi_env env, const char * format, ...)
{
	char buffer[1024];

	va_list args;
	va_start (args, format);
	vsnprintf (buffer, sizeof(buffer), format, args);
	//perror (buffer);
	va_end (args);
	napi_value s1;
	napi_create_string_utf8(env, buffer, strlen(buffer), &s1);
	napi_value global, console_obj, log;
	napi_get_global(env, &global);
	napi_get_named_property(env, global, "console", &console_obj);
	napi_get_named_property(env, console_obj, "log", &log);
	napi_call_function(env, global, log, 1, &s1, NULL);
}

static void log_var(napi_env env, const char * format, napi_value val)
{
	napi_value s1;
	napi_create_string_utf8(env, format, strlen(format), &s1);
	napi_value global, console_obj, log;
	napi_get_global(env, &global);
	napi_get_named_property(env, global, "console", &console_obj);
	napi_get_named_property(env, console_obj, "log", &log);
	napi_value args[] = {s1, val};
	napi_call_function(env, global, log, 2, args, NULL);
}


#define CALL_SAFE(env, the_call) \
	if ((the_call) != napi_ok) { \
		logs(env, "napi call error %s", #the_call); \
		napi_throw_error(env, NULL, "napi call error: " #the_call); \
	}

#define DECLARE_NAPI_PROPERTY(name, func)                                \
  { (name), 0, (func), 0, 0, 0, napi_default, 0 }

#define NAPI_ASSERT_BASE(env, assertion, message, ret_val)               \
  do {                                                                   \
    if (!(assertion)) {                                                  \
      napi_throw_error(                                                  \
          (env),                                                         \
        NULL,                                                            \
          "assertion (" #assertion ") failed: " message);                \
      return ret_val;                                                    \
    }                                                                    \
  } while (0)

// Returns NULL on failed assertion.
// This is meant to be used inside napi_callback methods.
#define NAPI_ASSERT(env, assertion, message)                             \
  NAPI_ASSERT_BASE(env, assertion, message, NULL)

// static char in_buff[1024];

// size_t unpack_underflow_handler(struct ccpcp_unpack_context *ctx)
// {
//	size_t n = fread(in_buff, 1, sizeof (in_buff), in_file);
//	ctx->start = ctx->current = in_buff;
//	ctx->end = ctx->start + n;
//	return n;
// }

// static char out_buff[1024];

// void pack_overflow_handler(struct ccpcp_pack_context *ctx, size_t size_hint)
// {
//	(void)size_hint;
//	fwrite(out_buff, 1, ctx->current - ctx->start, out_file);
//	ctx->start = ctx->current = out_buff;
//	ctx->end = ctx->start + sizeof (out_buff);
//}

typedef struct cpjs_var
{
	napi_value meta;
	napi_value val;
	ccpcp_item_types type;
} cpjs_var;

static void cpjs_var_init(cpjs_env *jsenv, cpjs_var *self)
{
	napi_get_undefined(jsenv->env, &self->meta);
	napi_get_undefined(jsenv->env, &self->val);
	self->type = CCPCP_ITEM_INVALID;
}

static bool cpjs_is_undefined(cpjs_env *jsenv, napi_value val)
{
	napi_valuetype t;
	CALL_SAFE(jsenv->env, napi_typeof(jsenv->env, val, &t));
	return (t == napi_undefined);
}

static void cpjs_string_concat(cpjs_env *jsenv, napi_value *val, const char *str, size_t len)
{
	napi_value s1;
	napi_create_string_utf8(jsenv->env, str, len, &s1);
	napi_value global, string_obj, prototype, concat;
	napi_get_global(jsenv->env, &global);
	napi_get_named_property(jsenv->env, global, "String", &string_obj);
	CALL_SAFE(jsenv->env, napi_get_named_property(jsenv->env, string_obj, "prototype", &prototype));
	napi_get_named_property(jsenv->env, prototype, "concat", &concat);
	napi_call_function(jsenv->env, *val, concat, 1, &s1, val);
}

static void cpjs_wrap_meta(cpjs_env *jsenv, cpjs_var *var, napi_value *result)
{
	const char *type_str = NULL;
	switch(var->type) {
	case CCPCP_ITEM_DATE_TIME:
		type_str = "DateTime";
		break;
	default:
		type_str = NULL;
		break;
	}
	bool has_meta = !cpjs_is_undefined(jsenv, var->meta);
	if(has_meta || type_str != NULL) {
		CALL_SAFE(jsenv->env, napi_create_object(jsenv->env, result));
		if(type_str != NULL) {
			napi_value str;
			CALL_SAFE(jsenv->env, napi_create_string_utf8(jsenv->env, type_str, NAPI_AUTO_LENGTH, &str));
			CALL_SAFE(jsenv->env, napi_set_named_property(jsenv->env, *result, "_type", str));
		}
		if(has_meta)
			CALL_SAFE(jsenv->env, napi_set_named_property(jsenv->env, *result, "_meta", var->meta));
		CALL_SAFE(jsenv->env, napi_set_named_property(jsenv->env, *result, "_val", var->val));
	}
	else {
		*result = var->val;
	}
}

static void cpjs_get_value(cpjs_env *jsenv, napi_value *val);

static void cpjs_add_list_item(cpjs_env *jsenv, napi_value list, napi_value val)
{
	napi_value global, array_obj, prototype, push;
	napi_get_global(jsenv->env, &global);
	CALL_SAFE(jsenv->env, napi_get_named_property(jsenv->env, global, "Array", &array_obj));
	CALL_SAFE(jsenv->env, napi_get_named_property(jsenv->env, array_obj, "prototype", &prototype));
	CALL_SAFE(jsenv->env, napi_get_named_property(jsenv->env, prototype, "push", &push));
	CALL_SAFE(jsenv->env, napi_call_function(jsenv->env, list, push, 1, &val, NULL));
}

static void cpjs_make_list(cpjs_env *jsenv, napi_value *val)
{
	CALL_SAFE(jsenv->env, napi_create_array(jsenv->env, val));
	while (true) {
		napi_value item;
		cpjs_get_value(jsenv, &item);
		if(jsenv->inCtx->item.type == CCPCP_ITEM_CONTAINER_END) {
			jsenv->inCtx->item.type = CCPCP_ITEM_INVALID; // to parse something like [[]]
			break;
		}
		cpjs_add_list_item(jsenv, *val, item);
	}
}

static void cpjs_make_map(cpjs_env *jsenv, napi_value *obj)
{
	CALL_SAFE(jsenv->env, napi_create_object(jsenv->env, obj));
	ccpon_unpack_next(jsenv->inCtx);
	while (true) {
		napi_value key;
		cpjs_get_value(jsenv, &key);
		if(jsenv->inCtx->item.type == CCPCP_ITEM_CONTAINER_END) {
			jsenv->inCtx->item.type = CCPCP_ITEM_INVALID;
			break;
		}
		napi_value val;
		cpjs_get_value(jsenv, &val);
		CALL_SAFE(jsenv->env, napi_set_property(jsenv->env, *obj, key, val));
	}
}

static void cpjs_make_decimal(cpjs_env *jsenv, cpjs_var *var, int64_t mantisa, int dec_places)
{
	double d = mantisa;
	d *= pow(10, -dec_places);
	napi_create_double(jsenv->env, d, &var->val);
}

static void cpjs_make_date_time(cpjs_env *jsenv, cpjs_var *var, int64_t msecs_since_epoch, int minutes_from_utc)
{
	var->type = CCPCP_ITEM_DATE_TIME;
	napi_value dt_msec_since_epoch;
	napi_value dt_minutes_from_utc;
	CALL_SAFE(jsenv->env, napi_create_object(jsenv->env, &var->val));
	CALL_SAFE(jsenv->env, napi_create_int64(jsenv->env, msecs_since_epoch, &dt_msec_since_epoch));
	CALL_SAFE(jsenv->env, napi_create_int32(jsenv->env, minutes_from_utc, &dt_minutes_from_utc));
	CALL_SAFE(jsenv->env, napi_set_named_property(jsenv->env, var->val, "msec", dt_msec_since_epoch));
	CALL_SAFE(jsenv->env, napi_set_named_property(jsenv->env, var->val, "tz", dt_minutes_from_utc));
}

static void cpjs_get_meta(cpjs_env *jsenv, napi_value *meta)
{
	const char *c = ccpon_unpack_skip_blank(jsenv->inCtx);
	jsenv->inCtx->current--;
	if(c && *c == '<') {
		CALL_SAFE(jsenv->env, napi_create_object(jsenv->env, meta));
		ccpon_unpack_next(jsenv->inCtx);
		while (true) {
			napi_value key;
			cpjs_get_value(jsenv, &key);
			if(jsenv->inCtx->item.type == CCPCP_ITEM_CONTAINER_END) {
				jsenv->inCtx->item.type = CCPCP_ITEM_INVALID;
				break;
			}
			napi_value val;
			cpjs_get_value(jsenv, &val);
			CALL_SAFE(jsenv->env, napi_set_property(jsenv->env, *meta, key, val));
		}
	}
	else {
		CALL_SAFE(jsenv->env, napi_get_undefined(jsenv->env, meta));
	}
}

static void cpjs_get_value(cpjs_env *jsenv, napi_value *val)
{
	cpjs_var var;
	cpjs_var_init(jsenv, &var);
	cpjs_get_meta(jsenv, &var.meta);

	ccpon_unpack_next(jsenv->inCtx);
	if(jsenv->inCtx->err_no != CCPCP_RC_OK)
		napi_throw_error(jsenv->env, NULL, "Native Cpon parse error");
		//PARSE_EXCEPTION("Cpon parse error: " + std::to_string(m_inCtx.err_no));

	ccpcp_item_types type = jsenv->inCtx->item.type;
	switch(type) {
	case CCPCP_ITEM_INVALID: {
		// end of input
		break;
	}
	case CCPCP_ITEM_LIST: {
		cpjs_make_list(jsenv, &var.val);
		break;
	}
	case CCPCP_ITEM_MAP: {
		cpjs_make_map(jsenv, &var.val);
		break;
	}
	case CCPCP_ITEM_IMAP: {
		cpjs_make_map(jsenv, &var.val);
		break;
	}
	case CCPCP_ITEM_CONTAINER_END: {
		break;
	}
	case CCPCP_ITEM_NULL: {
		CALL_SAFE(jsenv->env, napi_get_null(jsenv->env, &var.val));
		break;
	}
	case CCPCP_ITEM_STRING: {
		CALL_SAFE(jsenv->env, napi_create_string_utf8(jsenv->env, "", 0, &var.val));
		ccpcp_string *it = &(jsenv->inCtx->item.as.String);
		while(jsenv->inCtx->item.type == CCPCP_ITEM_STRING) {
			cpjs_string_concat(jsenv, &var.val, it->chunk_start, it->chunk_size);
			if(it->last_chunk)
				break;
			ccpon_unpack_next(jsenv->inCtx);
			if(jsenv->inCtx->err_no != CCPCP_RC_OK || jsenv->inCtx->err_no != CCPCP_ITEM_STRING)
				napi_throw_error(jsenv->env, NULL, "Native Cpon Unfinished string");
		}
		break;
	}
	case CCPCP_ITEM_BOOLEAN: {
		CALL_SAFE(jsenv->env, napi_get_boolean(jsenv->env, jsenv->inCtx->item.as.Bool, &var.val));
		break;
	}
	case CCPCP_ITEM_INT: {
		CALL_SAFE(jsenv->env, napi_create_int64(jsenv->env, jsenv->inCtx->item.as.Int, &var.val));
		break;
	}
	case CCPCP_ITEM_UINT: {
		CALL_SAFE(jsenv->env, napi_create_int64(jsenv->env, (int64_t)jsenv->inCtx->item.as.UInt, &var.val));
		break;
	}
	case CCPCP_ITEM_DECIMAL: {
		ccpcp_decimal *it = &(jsenv->inCtx->item.as.Decimal);
		cpjs_make_decimal(jsenv, &var, it->mantisa, it->dec_places);
		break;
	}
	case CCPCP_ITEM_DOUBLE: {
		CALL_SAFE(jsenv->env, napi_create_double(jsenv->env, jsenv->inCtx->item.as.Double, &var.val));
		break;
	}
	case CCPCP_ITEM_DATE_TIME: {
		ccpcp_date_time *it = &(jsenv->inCtx->item.as.DateTime);
		cpjs_make_date_time(jsenv, &var, it->msecs_since_epoch, it->minutes_from_utc);
		break;
	}
	default:
		napi_throw_error(jsenv->env, NULL, "Native Cpon Invalid type");
	}
	cpjs_wrap_meta(jsenv, &var, val);
}

napi_value parse_cpon_buffer(napi_env env, napi_callback_info info)
{
	size_t argc = 1;
	napi_value args[1];
	CALL_SAFE(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

	NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

	void* buffer_data;
	size_t buffer_length;

	CALL_SAFE(env, napi_get_buffer_info(env, args[0], &buffer_data, &buffer_length));

	log_var(env, "Incoming value", args[0]);
	/*
	static const size_t STATE_CNT = 100;
	ccpcp_container_state states[STATE_CNT];
	ccpcp_container_stack stack;
	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	*/
	ccpcp_unpack_context in_ctx;

	ccpcp_unpack_context_init(&in_ctx, buffer_data, buffer_length, NULL, NULL);

	cpjs_env jsenv1;
	cpjs_env *jsenv = &jsenv1;
	cpjs_env_init(jsenv, env, &in_ctx);

	//bool o_cpon_input = true; // (in_format == CCPCP_Cpon);

	napi_value ret_val;

	cpjs_get_value(jsenv, &ret_val);

	return ret_val;
}

static napi_value Init(napi_env env, napi_value exports) {
	napi_property_descriptor desc = DECLARE_NAPI_PROPERTY("parse_cpon_buffer", parse_cpon_buffer);
	napi_define_properties(env, exports, 1, &desc);
	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
