#pragma once

#include "appclioptions.h"

#include <shv/iotqt/rpc/password.h>
#include <shv/broker/brokerapp.h>

class BrokerApp : public shv::broker::BrokerApp
{
	Q_OBJECT
private:
	using Super = shv::broker::BrokerApp;
public:
	BrokerApp(int &argc, char **argv, AppCliOptions* cli_opts);
	~BrokerApp() Q_DECL_OVERRIDE;

	QString versionString() const;

	AppCliOptions* cliOptions() override;

	static BrokerApp* instance() {return qobject_cast<BrokerApp*>(Super::instance());}

	shv::iotqt::rpc::Password password(const std::string &user) override;
protected:
	std::set<std::string> aclUserFlattenRoles(const std::string &user_name) override;
	shv::chainpack::AclRole aclRole(const std::string &role_name) override;
	shv::chainpack::AclRolePaths aclRolePaths(const std::string &role_name) override;
};

