#include "application.h"

#include <shv/coreqt/log.h>
#include <shv/iotqt/utils/network.h>

#include <iostream>

int main(int argc, char *argv[])
{
	Application::registerLogTopics();

	std::vector<std::string> shv_args = NecroLog::setCLIOptions(argc, argv);

	shv::broker::AppCliOptions cli_opts;
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

	if(!cli_opts.loadConfigFile()) {
		return EXIT_FAILURE;
	}

	shv::chainpack::RpcMessage::registerMetaTypes();

	shvInfo() << "====================================================================";
	shvInfo() << "Starting shvbroker PID:" << QCoreApplication::applicationPid();
	shvInfo() << "--------------------------------------------------------------------";

	Application a(argc, argv, &cli_opts);

	return Application::exec();
}

