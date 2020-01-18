#pragma once

#include "appclioptions.h"

#include <shv/iotqt/rpc/password.h>
#include <shv/broker/brokerapp.h>

class SampleBrokerApp : public shv::broker::BrokerApp
{
	Q_OBJECT
private:
	using Super = shv::broker::BrokerApp;
public:
	SampleBrokerApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~SampleBrokerApp() Q_DECL_OVERRIDE;

	QString versionString() const;

	AppCliOptions* cliOptions() override;

	static SampleBrokerApp* instance() {return qobject_cast<SampleBrokerApp*>(Super::instance());}

};

