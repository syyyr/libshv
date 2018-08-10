#include <necrolog.h>
#include <ccpon.h>

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

size_t unpack_underflow_handler(struct ccpon_unpack_context *ctx, unsigned long)
{
	size_t n = ::fread(in_buff, 1, sizeof (in_buff), in_file);
	ctx->start = ctx->current = in_buff;
	ctx->end = ctx->start + n;
	return n;
}

static FILE *out_file = nullptr;
static uint8_t out_buff[1024];

size_t pack_overflow_handler(struct ccpon_pack_context *ctx, unsigned long)
{
	::fwrite(out_buff, 1, ctx->end - ctx->start, out_file);
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

	ccpon_unpack_context in_ctx;
	ccpon_unpack_context_init(&in_ctx, in_buff, 0, unpack_underflow_handler);

	ccpon_pack_context out_ctx;
	ccpon_pack_context_init(&out_ctx, out_buff, sizeof (out_buff), pack_overflow_handler);
	if(!o_indent.empty()) {
		out_ctx.indent = o_indent.data();
	}

	struct ContainerState
	{
		enum class ItemType {Field, Key, Val};
		ccpon_item_types containerType;
		ItemType itemType = ItemType::Field;
		int itemCount = 0;

		ContainerState(ccpon_item_types ct) : containerType(ct) {}
	};
	std::vector<ContainerState> cont_states;

	do {
		ccpon_unpack_next(&in_ctx);
		if(in_ctx.err_no != CCPON_RC_OK)
			break;
		switch(in_ctx.item.type) {
		case CCPON_ITEM_INVALID: {
			// end of input
			break;
		}
		case CCPON_ITEM_LIST: {
			ccpon_pack_list_begin(&out_ctx);
			ContainerState cs(in_ctx.item.type);
			cont_states.push_back(cs);
			break;
		}
		case CCPON_ITEM_ARRAY: {
			ccpon_pack_array_begin(&out_ctx);
			ContainerState cs(in_ctx.item.type);
			cont_states.push_back(cs);
			break;
		}
		case CCPON_ITEM_MAP: {
			ccpon_pack_map_begin(&out_ctx);
			ContainerState cs(in_ctx.item.type);
			cs.itemType = ContainerState::ItemType::Key;
			cont_states.push_back(cs);
			break;
		}
		case CCPON_ITEM_IMAP: {
			ccpon_pack_imap_begin(&out_ctx);
			ContainerState cs(in_ctx.item.type);
			cs.itemType = ContainerState::ItemType::Key;
			cont_states.push_back(cs);
			break;
		}
		case CCPON_ITEM_META: {
			ccpon_pack_meta_begin(&out_ctx);
			ContainerState cs(in_ctx.item.type);
			cs.itemType = ContainerState::ItemType::Key;
			cont_states.push_back(cs);
			break;
		}
		case CCPON_ITEM_CONTAINER_END:
		{
			if(cont_states.empty()) {
				nError() << "nesting error";
				in_ctx.err_no = CCPON_RC_MALFORMED_INPUT;
				break;
			}
			ContainerState &cs = cont_states[cont_states.size() - 1];
			if(cs.containerType == CCPON_ITEM_LIST || cs.containerType == CCPON_ITEM_ARRAY)
				ccpon_pack_list_end(&out_ctx);
			else if(cs.containerType == CCPON_ITEM_MAP || cs.containerType == CCPON_ITEM_IMAP)
				ccpon_pack_map_end(&out_ctx);
			else if(cs.containerType == CCPON_ITEM_META)
				ccpon_pack_meta_end(&out_ctx);
			else {
				nError() << "wrong container end";
				in_ctx.err_no = CCPON_RC_MALFORMED_INPUT;
			}
			cont_states.pop_back();
			if(!o_indent.empty())
				ccpon_pack_copy_str(&out_ctx, "\n", 1);
			break;
		}
		default: {
			if(in_ctx.item.type == CCPON_ITEM_STRING) {
				ccpon_string *it = &in_ctx.item.as.String;
				if(it->parse_status.chunk_cnt > 1) {
					// multichunk string
					// this can happen, when parsed string is greater than unpack_context buffer
					// concatenate with previous chunk
					ccpon_pack_copy_str(&out_ctx, it->start, it->length);
					if(it->parse_status.last_chunk) {
						ccpon_pack_copy_str(&out_ctx, "\"", 1);
					}
					break;
				}
			}
			if(!cont_states.empty()) {
				ContainerState &cs = cont_states[cont_states.size() - 1];
				switch(cs.itemType) {
				case ContainerState::ItemType::Field:
					ccpon_pack_field_delim(&out_ctx, cs.itemCount == 0);
					cs.itemCount++;
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
				//nError() << "nesting error";
				//in_ctx.err_no = CCPON_RC_MALFORMED_INPUT;
				//break;
			}
			switch(in_ctx.item.type) {
			case CCPON_ITEM_STRING: {
				ccpon_string *it = &in_ctx.item.as.String;
				//if(it->format == CCPON_STRING_FORMAT_HEX)
				//	ccpon_pack_copy_str(&out_ctx, "x", 1);
				ccpon_pack_copy_str(&out_ctx, "\"", 1);
				ccpon_pack_copy_str(&out_ctx, it->start, it->length);
				if(it->parse_status.last_chunk)
					ccpon_pack_copy_str(&out_ctx, "\"", 1);
				break;
			}
			case CCPON_ITEM_BOOLEAN: {
				ccpon_pack_boolean(&out_ctx, in_ctx.item.as.Bool);
				break;
			}
			case CCPON_ITEM_INT: {
				ccpon_pack_int(&out_ctx, in_ctx.item.as.Int);
				break;
			}
			case CCPON_ITEM_UINT: {
				ccpon_pack_uint(&out_ctx, in_ctx.item.as.UInt);
				break;
			}
			case CCPON_ITEM_DECIMAL: {
				ccpon_pack_decimal(&out_ctx, in_ctx.item.as.Decimal.mantisa, in_ctx.item.as.Decimal.dec_places);
				break;
			}
			case CCPON_ITEM_DOUBLE: {
				ccpon_pack_double(&out_ctx, in_ctx.item.as.Double);
				break;
			}
			case CCPON_ITEM_DATE_TIME: {
				ccpon_date_time *it = &in_ctx.item.as.DateTime;
				ccpon_pack_date_time(&out_ctx, it->msecs_since_epoch, it->minutes_from_utc);
				break;
			}
			default:
				ccpon_pack_null(&out_ctx);
				break;
			}
		}
		}
	} while(in_ctx.err_no == CCPON_RC_OK && out_ctx.err_no == CCPON_RC_OK);
	pack_overflow_handler(&out_ctx, 1);

	if(in_ctx.err_no != CCPON_RC_OK && in_ctx.err_no != CCPON_RC_BUFFER_UNDERFLOW)
		nError() << "Parse error:" << in_ctx.err_no;

	return 0;
}
