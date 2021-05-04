#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/cponreader.h>
#include <shv/chainpack/cponwriter.h>
#include <iostream>
#include <fstream>
#include <string.h>

static const char *cpmerge_help =
"\n"
"Cpmerge merge stdin with config.file. Merged file put to stdout\n"
"\n"
"USAGE: cpmerge config.file\n";

shv::chainpack::RpcValue::Map mergeRpcMap(shv::chainpack::RpcValue::Map site_config, shv::chainpack::RpcValue::Map template_config);

int main(int argc, char *argv[])
{
	if (argc < 2) {
		std::cerr << "No argument. Use config.file as first argument." << std::endl;
		std::cout << cpmerge_help;
		return 1;
	}

	for(int i=1; i<argc; i++) {
		const char *arg = argv[i];
		if(strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
			std::cout << cpmerge_help;
			return 0;
		}
	}

	std::ifstream fis;
	fis.open(argv[1]);
	if (!fis.good()) {
		std::cerr << "Cannot open config file for reading." << std::endl;
		return 1;
	}
	std::string err;
	shv::chainpack::RpcValue template_config = shv::chainpack::CponReader(fis).read(&err);
	fis.close();

	if (err.length() > 0){
		std::cerr << err << std::endl;
		return 2;
	}

	shv::chainpack::RpcValue site_config = shv::chainpack::CponReader(std::cin).read(&err);

	if (err.length() > 0){
		std::cerr << err << std::endl;
		return 3;
	}

	shv::chainpack::RpcValue::Map merged_config = mergeRpcMap(site_config.toMap(), template_config.toMap());
	shv::chainpack::RpcValue rv(merged_config);

	shv::chainpack::CponWriterOptions opts;
	opts.setIndent("\t");
	shv::chainpack::CponWriter wr(std::cout, opts);
	wr << rv;
	return 0;
}


shv::chainpack::RpcValue::Map mergeRpcMap(shv::chainpack::RpcValue::Map site_config, shv::chainpack::RpcValue::Map template_config)
{
	for(auto it : template_config.keys()) {
		if(site_config.hasKey(it)) {
			if(site_config.value(it).isMap() && template_config.value(it).isMap()) {
				shv::chainpack::RpcValue::Map one(site_config.value(it).toMap());
				shv::chainpack::RpcValue::Map two(template_config.value(it).toMap());
				one = mergeRpcMap(one, two);
				site_config[it] = one;
			}
		}
		else {
			site_config[it] = template_config.value(it);
		}
	}
	return site_config;
}
