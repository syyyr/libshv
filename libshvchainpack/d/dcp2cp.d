import shv.log;
import std.stdio;
import std.array;
import std.algorithm;

import shv.rpcvalue;
import shv.cpon;
import shv.chainpack;

enum cp2cp_help =
` [options] [input_file]

ChainPack to Cpon converter
USAGE:
-i "indent_string"
	indent Cpon, use "\t" for TAB (default is no-indent "")
-t
	human readable metatypes in Cpon output
--ip
	input stream is Cpon (ChainPack otherwise)
--oc
	write output in ChainPack (Cpon otherwise)
`;

void help(string app_name)
{
	import core.stdc.stdlib : exit;
	writeln(app_name, cp2cp_help);
	exit(0);
}

enum CHUNK_SIZE = 4 * 1024;

int main(string[] orig_args)
{
	string[] args = globalLog.setCLIOptions(orig_args);
	//logMessage("args:", args);

	string o_indent = "";
	//bool o_translate_meta_ids = false;
	bool o_cpon_input = false;
	bool o_chainpack_output = false;
	string file_name = null;

	string peek_arg() {return args.empty? "": args.front();}
	string get_arg() {
		if(args.empty)
			return "";
		auto ret = args.front();
		args.popFront();
		return ret;
	}
	string app_path = get_arg();
	string app_name() {
		import std.path;
		return baseName(stripExtension(app_path));
	}
	while(!args.empty()) {
		string arg = get_arg();

		if(arg == "-h" || arg == "--help")
			help(app_name);
		else if(arg == "-i") {
			o_indent = get_arg();
			if(o_indent == "\\t")
				o_indent = "\t";
		}
		//else if(!arg == "-t")
		//	o_translate_meta_ids = true;
		else if(arg == "--ip")
			o_cpon_input = true;
		else if(arg == "--oc")
			o_chainpack_output = true;
		else
			file_name = arg;
	}

	File in_file;
	if(file_name.length == 0) {
		in_file = stdin;
	}
	else {
		logMessage("opening file:", file_name);
		in_file = File(file_name, "r");
		//fprintf(stderr, "Cannot open '%s' for reading\n", file_name);
	}
	File out_file = stdout;

	auto in_range = in_file.byChunk(CHUNK_SIZE).joiner;

	auto out_range = out_file.lockingBinaryWriter;

	try {
		RpcValue val;
		if(o_cpon_input) {
			logMessage("reading Cpon");
			val = shv.cpon.read(in_range);
		}
		else {
			logMessage("reading ChainPack");
			val = shv.chainpack.read(in_range);
		}
		if(o_chainpack_output) {
			logMessage("writing Chainpack");
			shv.chainpack.write(out_range, val);
		}
		else {
			logMessage("writing Cpon");
			shv.cpon.WriteOptions opts;
			opts.indent = o_indent;
			shv.cpon.write(out_range, val);
		}
	}
	catch(Exception ex) {
			import core.stdc.stdlib : exit;
		writeln(ex);
		exit(-1);
	}

	return 0;
}