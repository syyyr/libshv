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

protected:
	/// ACL API, reimplemnt this functions if you do not want to use files in
	/// sampleshvbroker/etc/shv/shvbroker
	shv::iotqt::rpc::Password password(const std::string &user) override;
	std::set<std::string> aclUserFlattenRoles(const std::string &user_name) override;
	shv::chainpack::AclRole aclRole(const std::string &role_name) override;
	shv::chainpack::AclRolePaths aclRolePaths(const std::string &role_name) override;
};

