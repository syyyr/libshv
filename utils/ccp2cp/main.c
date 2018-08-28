#include <ccpon.h>
#include <cchainpack.h>

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
static uint8_t in_buff[1024];

size_t unpack_underflow_handler(struct ccpcp_unpack_context *ctx)
{
	size_t n = fread(in_buff, 1, sizeof (in_buff), in_file);
	ctx->start = ctx->current = in_buff;
	ctx->end = ctx->start + n;
	return n;
}

static FILE *out_file = NULL;
static uint8_t out_buff[1024];

size_t pack_overflow_handler(struct ccpcp_pack_context *ctx)
{
	fwrite(out_buff, 1, ctx->current - ctx->start, out_file);
	ctx->start = ctx->current = out_buff;
	ctx->end = ctx->start + sizeof (out_buff);
	return sizeof (out_buff);
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
			if(i < argc - 1)
				o_indent = argv[++i];
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
	ccpcp_container_stack stack;
	ccpc_container_stack_init(&stack, states, STATE_CNT, NULL);
	ccpcp_unpack_context in_ctx;
	ccpcp_unpack_context_init(&in_ctx, in_buff, 0, unpack_underflow_handler, &stack);

	ccpcp_pack_context out_ctx;
	ccpcp_pack_context_init(&out_ctx, out_buff, sizeof (out_buff), pack_overflow_handler);
	out_ctx.indent = o_indent;

	int prev_item = CCPCP_ITEM_INVALID;
	do {
		if(o_cpon_input)
			ccpon_unpack_next(&in_ctx);
		else
			cchainpack_unpack_next(&in_ctx);
		if(in_ctx.err_no != CCPCP_RC_OK)
			break;

		if(!o_chainpack_output) {
			ccpcp_container_state *curr_item_cont_state = ccpc_unpack_context_current_item_container_state(&in_ctx);
			if(curr_item_cont_state != NULL) {
				bool is_string_concat = 0;
				if(in_ctx.item.type == CCPCP_ITEM_STRING) {
					ccpcp_string *it = &in_ctx.item.as.String;
					if(it->parse_status.chunk_cnt > 1) {
						// multichunk string
						// this can happen, when parsed string is greater than unpack_context buffer
						// or escape sequence is encountered
						// concatenate it with previous chunk
						is_string_concat = 1;
					}
				}
				if(!is_string_concat && !ccpcp_item_type_is_container_end(in_ctx.item.type)) {
					switch(curr_item_cont_state->container_type) {
					case CCPCP_ITEM_LIST:
					case CCPCP_ITEM_ARRAY:
						if(prev_item != CCPCP_ITEM_META_END)
							ccpon_pack_field_delim(&out_ctx, curr_item_cont_state->item_count == 1);
						break;
					case CCPCP_ITEM_MAP:
					case CCPCP_ITEM_IMAP:
					case CCPCP_ITEM_META: {
						bool is_val = (curr_item_cont_state->item_count % 2) == 0;
						if(is_val) {
							ccpon_pack_key_delim(&out_ctx);
						}
						else {
							if(prev_item != CCPCP_ITEM_META_END)
								ccpon_pack_field_delim(&out_ctx, curr_item_cont_state->item_count == 1);
						}
						break;
					}
					default:
						break;
					}
				}
			}
		}
		switch(in_ctx.item.type) {
		case CCPCP_ITEM_INVALID: {
			// end of input
			break;
		}
		case CCPCP_ITEM_LIST: {
			if(o_chainpack_output)
				cchainpack_pack_list_begin(&out_ctx);
			else
				ccpon_pack_list_begin(&out_ctx);
			break;
		}
		case CCPCP_ITEM_ARRAY: {
			if(o_chainpack_output)
				cchainpack_pack_array_begin(&out_ctx, in_ctx.item.as.Array.size);
			else
				ccpon_pack_array_begin(&out_ctx, in_ctx.item.as.Array.size);
			break;
		}
		case CCPCP_ITEM_MAP: {
			if(o_chainpack_output)
				cchainpack_pack_map_begin(&out_ctx);
			else
				ccpon_pack_map_begin(&out_ctx);
			break;
		}
		case CCPCP_ITEM_IMAP: {
			if(o_chainpack_output)
				cchainpack_pack_imap_begin(&out_ctx);
			else
				ccpon_pack_imap_begin(&out_ctx);
			break;
		}
		case CCPCP_ITEM_META: {
			if(o_chainpack_output)
				cchainpack_pack_meta_begin(&out_ctx);
			else
				ccpon_pack_meta_begin(&out_ctx);
			break;
		}
		case CCPCP_ITEM_LIST_END: {
			if(o_chainpack_output)
				cchainpack_pack_list_end(&out_ctx);
			else
				ccpon_pack_list_end(&out_ctx);
			break;
		}
		case CCPCP_ITEM_ARRAY_END: {
			if(o_chainpack_output)
				;
			else
				ccpon_pack_list_end(&out_ctx);
			break;
		}
		case CCPCP_ITEM_MAP_END: {
			if(o_chainpack_output)
				cchainpack_pack_map_end(&out_ctx);
			else
				ccpon_pack_map_end(&out_ctx);
			break;
		}
		case CCPCP_ITEM_IMAP_END: {
			if(o_chainpack_output)
				cchainpack_pack_imap_end(&out_ctx);
			else
				ccpon_pack_imap_end(&out_ctx);
			break;
		}
		case CCPCP_ITEM_META_END: {
			if(o_chainpack_output)
				cchainpack_pack_meta_end(&out_ctx);
			else
				ccpon_pack_meta_end(&out_ctx);
			break;
		}
		case CCPCP_ITEM_STRING: {
			ccpcp_string *it = &in_ctx.item.as.String;
			if(o_chainpack_output) {
				if(it->parse_status.chunk_cnt == 1) {
					// first string chunk
					cchainpack_pack_uint(&out_ctx, it->length);
					ccpcp_pack_copy_bytes(&out_ctx, it->start, it->length);
				}
				else {
					// next string chunk
					ccpcp_pack_copy_bytes(&out_ctx, it->start, it->length);
				}
			}
			else {
				if(it->parse_status.chunk_cnt == 1) {
					// first string chunk
					ccpcp_pack_copy_bytes(&out_ctx, "\"", 1);
					ccpcp_pack_copy_bytes(&out_ctx, it->start, it->length);
				}
				else {
					// next string chunk
					ccpcp_pack_copy_bytes(&out_ctx, it->start, it->length);
				}
				if(it->parse_status.last_chunk)
					ccpcp_pack_copy_bytes(&out_ctx, "\"", 1);
			}
			break;
		}
		case CCPCP_ITEM_BOOLEAN: {
			ccpon_pack_boolean(&out_ctx, in_ctx.item.as.Bool);
			break;
		}
		case CCPCP_ITEM_INT: {
			ccpon_pack_int(&out_ctx, in_ctx.item.as.Int);
			break;
		}
		case CCPCP_ITEM_UINT: {
			ccpon_pack_uint(&out_ctx, in_ctx.item.as.UInt);
			break;
		}
		case CCPCP_ITEM_DECIMAL: {
			ccpon_pack_decimal(&out_ctx, in_ctx.item.as.Decimal.mantisa, in_ctx.item.as.Decimal.dec_places);
			break;
		}
		case CCPCP_ITEM_DOUBLE: {
			ccpon_pack_double(&out_ctx, in_ctx.item.as.Double);
			break;
		}
		case CCPCP_ITEM_DATE_TIME: {
			//static int n = 0;
			//printf("%d\n", n++);
			ccpcp_date_time *it = &in_ctx.item.as.DateTime;
			ccpon_pack_date_time(&out_ctx, it->msecs_since_epoch, it->minutes_from_utc);
			break;
		}
		default:
			ccpon_pack_null(&out_ctx);
			break;
		}
		prev_item = in_ctx.item.type;
	} while(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);
	pack_overflow_handler(&out_ctx);

	if(in_ctx.err_no != CCPCP_RC_OK && in_ctx.err_no != CCPCP_RC_BUFFER_UNDERFLOW)
		fprintf(stderr, "Parse error: %d\n", in_ctx.err_no);

	return 0;
}
