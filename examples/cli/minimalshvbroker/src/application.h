#pragma once

#include <shv/broker/brokerapp.h>

class Application : public shv::broker::BrokerApp
{
	Q_OBJECT
private:
	using Super = shv::broker::BrokerApp;
public:
	Application(int &argc, char **argv, shv::broker::AppCliOptions* cli_opts);
};

