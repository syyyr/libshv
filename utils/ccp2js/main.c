#include <ccpon.h>
#include <cchainpack.h>
#include <ccpcp_convert.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *cp2cp_help =
"\n"
"ChainPack to Cpon converter\n"
"\n"
"USAGE:\n"
"-i \"indent_string\"\n"
"	indent Cpon (default is no-indent \"\")\n"
"-t\n"
"	human readable metatypes in Cpon output\n"
"--ip\n"
"	input stream is Cpon (ChainPack otherwise)\n"
"--oc\n"
"	write output in ChainPack (Cpon otherwise)\n"
;
/*
int replace_str(std::string& str, const std::string& from, const std::string& to)
{
	int i = 0;
	for (i = 0; ; ++i) {
		size_t start_pos = str.find(from);
		if(start_pos == std::string::npos)
			break;
		str.replace(start_pos, from.length(), to);
	}
	return i;
}
*/
void help(const char *app_name)
{
	printf("%s%s", app_name, cp2cp_help);
	exit(0);
}

static FILE *in_file = NULL;
static char in_buff[1024];

size_t unpack_underflow_handler(struct ccpcp_unpack_context *ctx)
{
	size_t n = fread(in_buff, 1, sizeof (in_buff), in_file);
	ctx->start = ctx->current = in_buff;
	ctx->end = ctx->start + n;
	return n;
}

static FILE *out_file = NULL;
static char out_buff[1024];

void pack_overflow_handler(struct ccpcp_pack_context *ctx, size_t size_hint)
{
	(void)size_hint;
	fwrite(out_buff, 1, ctx->current - ctx->start, out_file);
	ctx->start = ctx->current = out_buff;
	ctx->end = ctx->start + sizeof (out_buff);
}

typedef struct js_var
{

} js_var;

static void js_var_init(js_var *self)
{
}

static void js_make_null(js_var *self)
{
}

static void js_make_bool(js_var *self, bool b)
{
}

static void js_make_int(js_var *self, int64_t i)
{
}

static void js_make_double(js_var *self, double i)
{
}

static void js_make_string_concat(js_var *self, const char *str, size_t len)
{
}

static void js_make_list(js_var *self)
{
}

static void js_make_map(js_var *self)
{
}

static void js_add_list_item(js_var *self, js_var *it)
{
}

static void js_make_map_key_concat(js_var *self, const char *str, size_t len)
{
}

static void js_add_map_val(js_var *self, js_var *val)
{
}

js_var *top_js_var(ccpcp_unpack_context *self)
{
	if(self->container_stack && self->container_stack->length > 0) {
		js_var *js_vars = (js_var *)self->custom_context;
		return js_vars + self->container_stack->length - 1;
	}
	return NULL;
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

void cpjs_convert(ccpcp_unpack_context* in_ctx, ccpcp_pack_format in_format, ccpcp_pack_context* out_ctx)
{
	bool o_cpon_input = (in_format == CCPCP_Cpon);
	//int prev_item = CCPCP_ITEM_INVALID;
	bool meta_just_closed = false;
	do {
		if(o_cpon_input)
			ccpon_unpack_next(in_ctx);
		else
			cchainpack_unpack_next(in_ctx);
		if(in_ctx->err_no != CCPCP_RC_OK)
			break;

		js_var *var = top_js_var(in_ctx);
		meta_just_closed = false;
		switch(in_ctx->item.type) {
		case CCPCP_ITEM_INVALID: {
			// end of input
			break;
		}
		case CCPCP_ITEM_NULL: {
			js_make_null(var);
			break;
		}
		case CCPCP_ITEM_LIST: {
			js_make_list(var);
			break;
		}
		case CCPCP_ITEM_META:
		case CCPCP_ITEM_MAP:
		case CCPCP_ITEM_IMAP: {
			js_make_map(var);
			break;
		}
		case CCPCP_ITEM_CONTAINER_END: {
			ccpcp_container_state *st = ccpcp_unpack_context_closed_container_state(in_ctx);
			if(!st) {
				in_ctx->err_no = CCPCP_RC_CONTAINER_STACK_UNDERFLOW;
				return;
			}
			meta_just_closed = (st->container_type == CCPCP_ITEM_META);
			break;
		}
		case CCPCP_ITEM_STRING: {
			ccpcp_string *it = &in_ctx->item.as.String;
			js_make_string_concat(var, it->chunk_start, it->chunk_size);
			break;
		}
		case CCPCP_ITEM_BOOLEAN: {
			js_make_bool(var, in_ctx->item.as.Bool);
			break;
		}
		case CCPCP_ITEM_INT: {
			js_make_int(var, in_ctx->item.as.Int);
			break;
		}
		case CCPCP_ITEM_UINT: {
			js_make_int(var, (int64_t)in_ctx->item.as.UInt);
			break;
		}
		case CCPCP_ITEM_DECIMAL: {
			// TODO convert decimal to double
			ccpcp_decimal *it = &in_ctx->item.as.Decimal;
			double d = 0;
			js_make_double(var, d);
			break;
		}
		case CCPCP_ITEM_DOUBLE: {
			js_make_double(var, in_ctx->item.as.Double);
			break;
		}
		case CCPCP_ITEM_DATE_TIME: {
			// TODO convert datetime to string
			ccpcp_date_time *it = &in_ctx->item.as.DateTime;
			js_make_string_concat(var, NULL, 0);
			break;
		}
		}

		js_var *parent_var = parent_js_var(in_ctx);
		ccpcp_container_state *parent_state = ccpcp_unpack_context_parent_container_state(in_ctx);
		if(parent_state && parent_var) {
			bool is_value_complete = false;
			switch(in_ctx->item.type) {
			case CCPCP_ITEM_STRING: {
				ccpcp_string *it = &(in_ctx->item.as.String);
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
					js_add_list_item(parent_var, var);
					break;
				case CCPCP_ITEM_MAP:
				case CCPCP_ITEM_IMAP:
				case CCPCP_ITEM_META: {
					bool is_key = (parent_state->item_count % 2);
					if(is_key) {
						if(in_ctx->item.type == CCPCP_ITEM_STRING) {
							ccpcp_string *it = &(in_ctx->item.as.String);
							js_make_map_key_concat(parent_var, it->chunk_start, it->chunk_size);
						}
						else if(in_ctx->item.type == CCPCP_ITEM_INT) {
							// TODO convert int to string
							js_make_map_key_concat(parent_var, 0, 0);
						}
						else if(in_ctx->item.type == CCPCP_ITEM_UINT) {
							// TODO convert int to string
							js_make_map_key_concat(parent_var, 0, 0);
						}
						else {
							// invalid key type
							//assert(false);
						}
					}
					else {
						js_add_map_val(parent_var, var);
					}
					break;
				}
				default:
					break;
				}
			}
		}

		{
			ccpcp_container_state *top_state = ccpcp_unpack_context_top_container_state(in_ctx);
			// take just one object from stream
			if(!top_state) {
				if((in_ctx->item.type == CCPCP_ITEM_STRING && !in_ctx->item.as.String.last_chunk)
						|| meta_just_closed) {
					// do not stop parsing in the middle of the string
					// or after meta
				}
				else {
					break;
				}
			}
		}
	} while(in_ctx->err_no == CCPCP_RC_OK && out_ctx->err_no == CCPCP_RC_OK);
}

int main(int argc, char *argv[])
{
	const char *o_indent = NULL;
	bool o_translate_meta_ids = false;
	bool o_cpon_input = false;
	bool o_chainpack_output = false;
	const char *file_name = NULL;

	for(int i=1; i<argc; i++) {
		const char *arg = argv[i];
		if(strcmp(arg, "-h") == 0)
			help(argv[0]);
		if(strcmp(arg, "--help") == 0)
			help(argv[0]);

		if(!strcmp(arg, "-i")) {
			if(i < argc - 1) {
				o_indent = argv[++i];
				if(!strcmp(o_indent, "\\t"))
					o_indent = "\t";
				//fprintf(stderr, "indent: >>>%s<<<\n", o_indent);
			}
		}
		else if(!strcmp(arg, "-t"))
			o_translate_meta_ids = true;
		else if(!strcmp(arg, "--ip"))
			o_cpon_input = true;
		else if(!strcmp(arg, "--oc"))
			o_chainpack_output = true;
		else if(!strcmp(arg, "-h") || !strcmp(arg, "--help"))
			help(argv[0]);
		else
			file_name = arg;
	}

	if(!file_name) {
		in_file = stdin;
	}
	else {
		in_file = fopen(file_name, "rb");
		if(!in_file) {
			fprintf(stderr, "Cannot open '%s' for reading\n", file_name);
			exit(-1);
		}
	}

	out_file = stdout;

	static const size_t STATE_CNT = 100;
	ccpcp_container_state states[STATE_CNT];
	js_var js_vars[STATE_CNT];
	ccpcp_container_stack stack;
	ccpcp_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context in_ctx;
	ccpcp_unpack_context_init(&in_ctx, in_buff, 0, unpack_underflow_handler, &stack);
	in_ctx.custom_context = js_vars;

	ccpcp_pack_context out_ctx;
	ccpcp_pack_context_init(&out_ctx, out_buff, sizeof (out_buff), pack_overflow_handler);
	out_ctx.cpon_options.indent = o_indent;

	cpjs_convert(&in_ctx, o_cpon_input? CCPCP_Cpon: CCPCP_ChainPack, &out_ctx);

	if(in_ctx.err_no != CCPCP_RC_OK && in_ctx.err_no != CCPCP_RC_BUFFER_UNDERFLOW)
		fprintf(stderr, "Parse error: %d\n", in_ctx.err_no);

	return 0;
}
