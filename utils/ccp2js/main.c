#include "./ccpon.h"
#include "./cchainpack.h"
#include "./ccpcp_convert.h"

#include "./common.h"

#include <node_api.h>
#include <assert.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>


static void log(napi_env env, const char * format, ...)
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
	// napi_get_named_property(env, global, "Array", &array_obj);
	napi_get_named_property(env, console_obj, "log", &log);
	// napi_get_named_property(env, array_obj, "push", &push);
	// napi_create_array(env, &array);
	// napi_call_function(env, args, push, 1, &s1, NULL);
	// napi_call_function(env, args, push, 1, &val, NULL);
	napi_value args[] = {s1, val};
	napi_call_function(env, global, log, 2, args, NULL);
}


#define CALL_SAFE(env, the_call) \
	if ((the_call) != napi_ok) { \
		log(env, "napi call error %s", #the_call); \
	}

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

typedef struct js_var
{
	napi_value val;
	napi_value map_key;
} js_var;

static void js_make_null(napi_env env, js_var *self)
{
	napi_get_null(env, &(self->val));
}

static void js_var_init(napi_env env, js_var *self)
{
	js_make_null(env, self);
	napi_get_null(env, &self->map_key);
}

static void js_make_bool(napi_env env, js_var *self, bool b)
{
	napi_get_boolean(env, b, &self->val);
}

static void js_make_int(napi_env env, js_var *self, int64_t i)
{
	//log(env, "%lld", i);
	napi_create_int64(env, i, &(self->val));
}

static void js_make_double(napi_env env, js_var *self, double i)
{
	napi_create_double(env, i, &self->val);
}

static void js_make_string_concat(napi_env env, js_var *self, const char *str, size_t len)
{
	log_var(env, "current result: ", self->val);

	napi_valuetype t;
	napi_typeof(env, self->val, &t);
	if (t == napi_null) {
		log_var(env, "string 2:", self->val);
		CALL_SAFE(env, napi_create_string_utf8(env, str, len, &self->val));
		log_var(env, "string 3:", self->val);
	}
	else {
		log_var(env, "string 4:", self->val);

		napi_value s1;
		napi_create_string_utf8(env, str, len, &s1);
		napi_value global, string_obj, prototype, concat;
		napi_get_global(env, &global);
		napi_get_named_property(env, global, "String", &string_obj);
		CALL_SAFE(env, napi_get_named_property(env, string_obj, "prototype", &prototype));
		napi_get_named_property(env, prototype, "concat", &concat);
		napi_call_function(env, self->val, concat, 1, &s1, &self->val);
	}
}

static void js_make_list(napi_env env, js_var *self)
{
	napi_create_array(env, &(self->val));
	log_var(env, "4: ", self->val);
}

static void js_make_map(napi_env env, js_var *self)
{
	napi_create_object(env, &self->val);
}

static void js_add_list_item(napi_env env, js_var *self, js_var *it)
{
	napi_value global, array_obj, prototype, push;
	napi_get_global(env, &global);
	CALL_SAFE(env, napi_get_named_property(env, global, "Array", &array_obj));
	CALL_SAFE(env, napi_get_named_property(env, array_obj, "prototype", &prototype));
	CALL_SAFE(env, napi_get_named_property(env, prototype, "push", &push));
	CALL_SAFE(env, napi_call_function(env, self->val, push, 1, &it->val, NULL));
}

static void js_make_map_key_concat(napi_env env, js_var *self, const char *str, size_t len)
{

	log_var(env, "string key 1: ", self->map_key);
	napi_valuetype t;
	napi_typeof(env, self->map_key, &t);
	if (t == napi_null) {
		napi_create_string_utf8(env, str, len, &self->map_key);
		log_var(env, "string key 2: ", self->map_key);
	}
	else {
		napi_value s1;
		napi_create_string_utf8(env, str, len, &s1);
		napi_value global, string_obj, prototype, concat;
		napi_get_global(env, &global);
		napi_get_named_property(env, global, "String", &string_obj);
		CALL_SAFE(env, napi_get_named_property(env, string_obj, "prototype", &prototype));
		napi_get_named_property(env, prototype, "concat", &concat);
		napi_call_function(env, self->map_key, concat, 1, &s1, &self->map_key);
	}
}

//static napi_env global_env;

static void js_make_map_int_key(napi_env env, js_var *self, int64_t key)
{
	log(env, "making key %lld ", key);
	napi_create_int64(env, key, &(self->map_key));
	napi_set_property(env, &self, "val", NULL);
}

static void js_add_map_val(napi_env env, js_var *self, js_var *val)
{
	log_var(env, "add map val", val->val);
	napi_set_property(env, self->val, self->map_key, val->val);

	log_var(env, "result", self->val);
}

js_var *top_js_var(ccpcp_unpack_context *self)
{
	js_var *js_vars = (js_var *)self->custom_context;
	if(self->container_stack && self->container_stack->length > 0) {
		ccpcp_container_state *top_st = self->container_stack->container_states + self->container_stack->length - 1;
		if(top_st && top_st->item_count == 0) {
			if(self->container_stack->length > 0)
				return js_vars + self->container_stack->length - 1;
			else
				return NULL;
		}
		return js_vars + self->container_stack->length;
	}
	return js_vars;
}

js_var *parent_js_var(ccpcp_unpack_context *self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		js_var *js_vars = (js_var *)self->custom_context;
		ccpcp_container_state *top_st = self->container_stack->container_states + self->container_stack->length - 1;
		if(top_st && top_st->item_count == 0) {
			if(self->container_stack->length > 1)
				return js_vars + self->container_stack->length - 2;
			else
				return NULL;
		}
		return js_vars + self->container_stack->length - 1;
	}
	return NULL;
}

napi_value parse_cpon_buffer(napi_env env, napi_callback_info info)
{
	//global_env = env;
	size_t argc = 1;
	napi_value args[1];
	NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

	NAPI_ASSERT(env, argc >= 1, "Wrong number of arguments");

	log_var(env, "Incoming value", args[0]);

	static const size_t STATE_CNT = 100;
	ccpcp_container_state states[STATE_CNT];
	js_var js_vars[STATE_CNT];
	ccpcp_container_stack stack;
	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context in_ctx;

	char* buffer_data;
	size_t buffer_length;
	NAPI_CALL(env, napi_get_buffer_info(env, args[0], &buffer_data, &buffer_length));

	ccpcp_unpack_context_init(&in_ctx, buffer_data, buffer_length, NULL, &stack);
	in_ctx.custom_context = js_vars;


	bool o_cpon_input = true; // (in_format == CCPCP_Cpon);

	js_var_init(env, &js_vars[0]);
	bool meta_just_closed = false;
	do {

		if(o_cpon_input)
			ccpon_unpack_next(&in_ctx);
		else
			cchainpack_unpack_next(&in_ctx);
		if(in_ctx.err_no != CCPCP_RC_OK)
			break;

		js_var *var = top_js_var(&in_ctx);

		meta_just_closed = false;
		switch(in_ctx.item.type) {
		case CCPCP_ITEM_INVALID: {
			// end of input
			break;
		}
		case CCPCP_ITEM_NULL: {
			js_make_null(env, var);
			break;
		}
		case CCPCP_ITEM_LIST: {
			js_make_list(env, var);
			break;
		}
		case CCPCP_ITEM_META:
		case CCPCP_ITEM_MAP:
		case CCPCP_ITEM_IMAP: {
			js_make_map(env, var);
			break;
		}
		case CCPCP_ITEM_CONTAINER_END: {
			ccpcp_container_state *st = ccpcp_unpack_context_closed_container_state(&in_ctx);
			if(!st) {
				in_ctx.err_no = CCPCP_RC_CONTAINER_STACK_UNDERFLOW;
				goto error;
			}
			meta_just_closed = (st->container_type == CCPCP_ITEM_META);
			break;
		}
		case CCPCP_ITEM_STRING: {
			ccpcp_string *it = &in_ctx.item.as.String;
			if (it->chunk_cnt == 1) {
				js_var_init(env, var);
			}
			js_make_string_concat(env, var, it->chunk_start, it->chunk_size);
			break;
		}
		case CCPCP_ITEM_BOOLEAN: {
			js_make_bool(env, var, in_ctx.item.as.Bool);
			break;
		}
		case CCPCP_ITEM_INT: {
			js_make_int(env, var, in_ctx.item.as.Int);
			break;
		}
		case CCPCP_ITEM_UINT: {
			js_make_int(env, var, (int64_t)in_ctx.item.as.UInt);
			break;
		}
		case CCPCP_ITEM_DECIMAL: {
			// TODO convert decimal to double
			ccpcp_decimal *it = &in_ctx.item.as.Decimal;
			double d = 0;
			js_make_double(env, var, d);
			break;
		}
		case CCPCP_ITEM_DOUBLE: {
			js_make_double(env, var, in_ctx.item.as.Double);
			break;
		}
		case CCPCP_ITEM_DATE_TIME: {
			// TODO convert datetime to string
			ccpcp_date_time *it = &in_ctx.item.as.DateTime;
			js_make_string_concat(env, var, it, 0);
			break;
		}
		}

		log_var(env, "1: ", js_vars[0].val);


		js_var *parent_var = parent_js_var(&in_ctx);
		ccpcp_container_state *parent_state = ccpcp_unpack_context_parent_container_state(&in_ctx);
		if(parent_state && parent_var) {
			bool is_value_complete = false;
			switch(in_ctx.item.type) {
			case CCPCP_ITEM_STRING: {
				ccpcp_string *it = &(in_ctx.item.as.String);
				is_value_complete = (it->last_chunk);
			}
			case CCPCP_ITEM_LIST:
				break;
			case CCPCP_ITEM_MAP:
			case CCPCP_ITEM_IMAP:
			case CCPCP_ITEM_META:
				break;
			default:
				is_value_complete = true;
				break;
			}
			if(is_value_complete) {
				switch(parent_state->container_type) {
				case CCPCP_ITEM_LIST:
					js_add_list_item(env, parent_var, var);
					break;
				case CCPCP_ITEM_MAP:
				case CCPCP_ITEM_IMAP:
				case CCPCP_ITEM_META: {
					bool is_key = (parent_state->item_count % 2);
					if(is_key) {
						if(in_ctx.item.type == CCPCP_ITEM_STRING) {
							ccpcp_string *it = &(in_ctx.item.as.String);
							js_make_map_key_concat(env, parent_var, it->chunk_start, it->chunk_size);
						}
						else if(in_ctx.item.type == CCPCP_ITEM_INT) {
							js_make_map_int_key(env, parent_var, in_ctx.item.as.Int);
						}
						else if(in_ctx.item.type == CCPCP_ITEM_UINT) {
							js_make_map_int_key(env, parent_var, in_ctx.item.as.UInt);
						}
						else {
							// invalid key type
							//assert(false);
						}
					}
					else {
						js_add_map_val(env, parent_var, var);
					}
					break;
				}
				default:
					break;
				}
			}
		}

		log_var(env, "2: ", js_vars[0].val);

		{
			ccpcp_container_state *top_state = ccpcp_unpack_context_top_container_state(&in_ctx);
			// take just one object from stream
			if(!top_state) {
				if((in_ctx.item.type == CCPCP_ITEM_STRING && !in_ctx.item.as.String.last_chunk)
						|| meta_just_closed) {
					// do not stop parsing in the middle of the string
					// or after meta
				}
				else {
					break;
				}
			}
		}
	} while(in_ctx.err_no == CCPCP_RC_OK);
	error:
	return js_vars[0].val;
}

static napi_value Init(napi_env env, napi_value exports) {
	napi_property_descriptor desc = DECLARE_NAPI_PROPERTY("parse_cpon_buffer", parse_cpon_buffer);
	NAPI_CALL(env, napi_define_properties(env, exports, 1, &desc));
	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
