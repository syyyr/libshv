#include <necrolog.h>
#include <ccpon.h>
#include <cchainpack.h>

#include <numeric>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

/*
#define nFooInfo() nCInfo("foo")
#define nBarDebug() nCDebug("bar")

inline NecroLog &operator<<(NecroLog log, const std::vector<std::string> &sl)
{
	std::string s = std::accumulate(sl.begin(), sl.end(), std::string(),
										  [](const std::string& a, const std::string& b) -> std::string {
											  return a + (a.length() > 0 ? "," : "") + b;
										  } );
	return log << s;
}
*/
static const char *cp2cp_help =
R"( ChainPack to Cpon converter

USAGE:
-i "indent_string"
	indent Cpon (default is no-indent "")
-t
	human readable metatypes in Cpon output
--ip
	input stream is Cpon (ChainPack otherwise)
--oc
	write output in ChainPack (Cpon otherwise)

)";

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

void help(const std::string &app_name)
{
	std::cout << app_name << cp2cp_help;
	std::cout << NecroLog::cliHelp();
	exit(0);
}

static FILE *in_file = nullptr;
static uint8_t in_buff[1024];

size_t unpack_underflow_handler(struct ccpcp_unpack_context *ctx, unsigned long)
{
	size_t n = ::fread(in_buff, 1, sizeof (in_buff), in_file);
	ctx->start = ctx->current = in_buff;
	ctx->end = ctx->start + n;
	return n;
}

static FILE *out_file = nullptr;
static uint8_t out_buff[1024];

size_t pack_overflow_handler(struct ccpcp_pack_context *ctx, unsigned long)
{
	::fwrite(out_buff, 1, ctx->current - ctx->start, out_file);
	ctx->start = ctx->current = out_buff;
	ctx->end = ctx->start + sizeof (out_buff);
	return sizeof (out_buff);
}

int main(int argc, char *argv[])
{
	std::vector<std::string> args = NecroLog::setCLIOptions(argc, argv);

	if(std::find(args.begin(), args.end(), "--help") != args.end()) {
		help(argv[0]);
	}
	nDebug() << NecroLog::tresholdsLogInfo();

	std::string o_indent;
	bool o_translate_meta_ids = false;
	bool o_cpon_input = false;
	bool o_chainpack_output = false;
	std::string file_name;

	for (size_t i = 1; i < args.size(); ++i) {
		const std::string &arg = args[i];
		if(arg == "-i" && i < args.size() - 1) {
			std::string s = argv[++i];
			//replace_str(s, "\\t", "\t");
			//replace_str(s, "\\r", "\r");
			//replace_str(s, "\\n", "\n");
			o_indent = s;
		}
		else if(arg == "-t")
			o_translate_meta_ids = true;
		else if(arg == "--ip")
			o_cpon_input = true;
		else if(arg == "--oc")
			o_chainpack_output = true;
		else if(arg == "-h" || arg == "--help")
			help(argv[0]);
		else
			file_name = arg;
	}

	if(file_name.empty()) {
		nDebug() << "reading stdin";
		in_file = stdin;
	}
	else {
		nDebug() << "reading:" << file_name;
		in_file = ::fopen(file_name.c_str(), "rb");
		if(!in_file) {
			nError() << "Cannot open" << file_name << "for reading";
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
	if(!o_indent.empty()) {
		out_ctx.indent = o_indent.data();
	}

	struct ContainerState
	{
		enum class ItemType {Field, Key, Val};
		ccpcp_item_types containerType;
		ItemType itemType = ItemType::Field;
		int itemCount = 0;

		ContainerState(ccpcp_item_types ct) : containerType(ct) {}
	};
	std::vector<ContainerState> cont_states;

	int meta_closed = false;
	do {
		if(o_cpon_input)
			ccpon_unpack_next(&in_ctx);
		else
			cchainpack_unpack_next(&in_ctx);
		if(in_ctx.err_no != CCPCP_RC_OK)
			break;
		if(!o_chainpack_output) {
			if(!cont_states.empty()) {
				bool is_string_concat = 0;
				if(in_ctx.item.type == CCPCP_ITEM_STRING) {
					ccpcp_string *it = &in_ctx.item.as.String;
					if(it->parse_status.chunk_cnt > 1) {
						// multichunk string
						// this can happen, when parsed string is greater than unpack_context buffer
						// concatenate with previous chunk
						is_string_concat = 1;
					}
				}
				if(!is_string_concat && in_ctx.item.type != CCPCP_ITEM_CONTAINER_END) {
					ContainerState &cs = cont_states[cont_states.size() - 1];
					switch(cs.itemType) {
					case ContainerState::ItemType::Field:
						if(!meta_closed) {
							ccpon_pack_field_delim(&out_ctx, cs.itemCount == 0);
							cs.itemCount++;
						}
						break;
					case ContainerState::ItemType::Key:
						ccpon_pack_field_delim(&out_ctx, cs.itemCount == 0);
						cs.itemType = ContainerState::ItemType::Val;
						break;
					case ContainerState::ItemType::Val:
						ccpon_pack_key_delim(&out_ctx);
						cs.itemType = ContainerState::ItemType::Key;
						cs.itemCount++;
						break;
					}
				}
			}
			meta_closed = false;
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
			ContainerState cs(in_ctx.item.type);
			cont_states.push_back(cs);
			break;
		}
		case CCPCP_ITEM_ARRAY: {
			if(o_chainpack_output)
				cchainpack_pack_array_begin(&out_ctx, in_ctx.item.as.Array.size);
			else
				ccpon_pack_array_begin(&out_ctx, in_ctx.item.as.Array.size);
			ContainerState cs(in_ctx.item.type);
			cont_states.push_back(cs);
			break;
		}
		case CCPCP_ITEM_MAP: {
			if(o_chainpack_output)
				cchainpack_pack_map_begin(&out_ctx);
			else
				ccpon_pack_map_begin(&out_ctx);
			ContainerState cs(in_ctx.item.type);
			cs.itemType = ContainerState::ItemType::Key;
			cont_states.push_back(cs);
			break;
		}
		case CCPCP_ITEM_IMAP: {
			if(o_chainpack_output)
				cchainpack_pack_imap_begin(&out_ctx);
			else
				ccpon_pack_imap_begin(&out_ctx);
			ContainerState cs(in_ctx.item.type);
			cs.itemType = ContainerState::ItemType::Key;
			cont_states.push_back(cs);
			break;
		}
		case CCPCP_ITEM_META: {
			if(o_chainpack_output)
				cchainpack_pack_meta_begin(&out_ctx);
			else
				ccpon_pack_meta_begin(&out_ctx);
			ContainerState cs(in_ctx.item.type);
			cs.itemType = ContainerState::ItemType::Key;
			cont_states.push_back(cs);
			break;
		}
		case CCPCP_ITEM_CONTAINER_END:
		{
			if(cont_states.empty()) {
				nError() << "nesting error";
				in_ctx.err_no = CCPCP_RC_MALFORMED_INPUT;
				break;
			}
			ContainerState &cs = cont_states[cont_states.size() - 1];
			if(o_chainpack_output) {
				if(cs.containerType == CCPCP_ITEM_LIST)
					cchainpack_pack_list_end(&out_ctx);
				else if(cs.containerType == CCPCP_ITEM_ARRAY)
					cchainpack_pack_array_end(&out_ctx);
				else if(cs.containerType == CCPCP_ITEM_MAP)
					cchainpack_pack_map_end(&out_ctx);
				else if(cs.containerType == CCPCP_ITEM_IMAP)
					cchainpack_pack_imap_end(&out_ctx);
				else if(cs.containerType == CCPCP_ITEM_META)
					cchainpack_pack_meta_end(&out_ctx);
				else {
					nError() << "wrong container end";
					in_ctx.err_no = CCPCP_RC_MALFORMED_INPUT;
				}
			}
			else {
				meta_closed = (cs.containerType == CCPCP_ITEM_META);
				if(cs.containerType == CCPCP_ITEM_LIST)
					ccpon_pack_list_end(&out_ctx);
				else if(cs.containerType == CCPCP_ITEM_ARRAY)
					ccpon_pack_array_end(&out_ctx);
				else if(cs.containerType == CCPCP_ITEM_MAP)
					ccpon_pack_map_end(&out_ctx);
				else if(cs.containerType == CCPCP_ITEM_IMAP)
					ccpon_pack_imap_end(&out_ctx);
				else if(cs.containerType == CCPCP_ITEM_META)
					ccpon_pack_meta_end(&out_ctx);
				else {
					nError() << "wrong container end";
					in_ctx.err_no = CCPCP_RC_MALFORMED_INPUT;
				}
			}
			cont_states.pop_back();
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
	} while(in_ctx.err_no == CCPCP_RC_OK && out_ctx.err_no == CCPCP_RC_OK);
	pack_overflow_handler(&out_ctx, 1);

	if(in_ctx.err_no != CCPCP_RC_OK && in_ctx.err_no != CCPCP_RC_BUFFER_UNDERFLOW)
		nError() << "Parse error:" << in_ctx.err_no;

	return 0;
}
