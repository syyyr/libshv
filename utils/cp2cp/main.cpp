#include <necrolog.h>

#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>

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
-i "indent_string" : indent Cpon
-t : human readable metatypes
-r : convert Cpon to ChainPack

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

int main(int argc, char *argv[])
{
	std::vector<std::string> args = NecroLog::setCLIOptions(argc, argv);

	if(std::find(args.begin(), args.end(), "--help") != args.end()) {
		std::cout << argv[0] << cp2cp_help;
		std::cout << NecroLog::cliHelp();
		exit(0);
	}
	nDebug() << NecroLog::tresholdsLogInfo();

	std::string o_indent;
	bool o_translate_meta_ids = false;
	bool o_cpon_to_chainpack = false;
	std::string file_name;

	for (size_t i = 1; i < args.size(); ++i) {
		const std::string &arg = args[i];
		if(arg == "-i" && i < args.size() - 1) {
			std::string s = argv[++i];
			replace_str(s, "\\t", "\t");
			replace_str(s, "\\r", "\r");
			replace_str(s, "\\n", "\n");
			o_indent = s;
		}
		else if(arg == "-t")
			o_translate_meta_ids = true;
		else if(arg == "-r")
			o_cpon_to_chainpack = true;
		else
			file_name = arg;
	}

	std::istream *pin = nullptr;
	std::ifstream in_file;
	if(file_name.empty()) {
		nDebug() << "reading stdin";
		pin = &std::cin;
	}
	else {
		nDebug() << "reading:" << file_name;
		in_file.open(file_name, std::ios::in | std::ios::binary);
		if(!in_file) {
			nError() << "Cannot open" << file_name << "for reading";
			exit(-1);
		}
		pin = &in_file;
	}

	try {
		if(o_cpon_to_chainpack) {
			nDebug() << "converting Cpon --> ChainPack";
			shv::chainpack::CponReader rd(*pin);
			shv::chainpack::ChainPackWriter wr(std::cout);
			while(true) {
				// read garbage to discover end of stream
				while(true) {
					int c = pin->get();
					if(c < 0)
						return 0;
					if(c > ' ') {
						pin->unget();
						break;
					}
				}
				shv::chainpack::RpcValue val = rd.read();
				//nInfo() << val.toCpon();
				wr.write(val);
			}
		}
		else {
			nDebug() << "converting ChainPack --> Cpon";
			shv::chainpack::ChainPackReader rd(*pin);
			shv::chainpack::CponWriterOptions opts;
			opts.setIndent(o_indent);
			opts.setTranslateIds(o_translate_meta_ids);
			shv::chainpack::CponWriter wr(std::cout, opts);
			while(true) {
				// check end of stream
				int c = pin->get();
				if(c < 0)
					break;
				pin->unget();
				shv::chainpack::RpcValue val = rd.read();
				wr.write(val);
			}
		}
	}
	catch (std::exception &e) {
		nError() << e.what();
		exit(-1);
	}

	return 0;
}
