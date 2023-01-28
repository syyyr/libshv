#include "application.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/rpc/clientappclioptions.h>

#include <iostream>

int main(int argc, char *argv[])
{
	// init log system
	NecroLog::setTopicsLogThresholds(":M");
	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);

	// check CLI argumnets
	shv::iotqt::rpc::ClientAppCliOptions cli_opts;
	cli_opts.parse(shv_args);
	if(cli_opts.isParseError()) {
		for(const std::string &err : cli_opts.parseErrors())
			shvError() << err;
		return EXIT_FAILURE;
	}
	if(cli_opts.isAppBreak()) {
		if(cli_opts.isHelp()) {
			cli_opts.printHelp(std::cout);
		}
		return EXIT_SUCCESS;
	}
	for(const std::string &s : cli_opts.unusedArguments()) {
		shvError() << "Undefined argument:" << s;
	}

	// create application object
	Application a(argc, argv, cli_opts);

	// start event-loop
	return a.exec();
}

