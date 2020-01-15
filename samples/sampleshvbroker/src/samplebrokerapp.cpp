#include "samplebrokerapp.h"

#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/log.h>
#include <shv/chainpack/aclrolepaths.h>

namespace cp = shv::chainpack;

static const char METH_STATUS[] = "status";
static const char METH_RESTART[] = "restart";

class TestNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	TestNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods{
			{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
			{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
			{METH_STATUS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Read},
			{METH_RESTART, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Command},
		}
	{ }

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override
	{
		if(shv_path.empty()) {
			if(method == METH_STATUS) {
				return cp::RpcValue{"STATUS"};
			}
			if(method == METH_RESTART) {
				return cp::RpcValue{"METH_RESTART"};
			}
		}
		return Super::callMethod(shv_path, method, params);
	}
private:
	std::vector<cp::MetaMethod> m_metaMethods;
};

SampleBrokerApp::SampleBrokerApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv, cli_opts)
{
	m_nodesTree->mount("test", new TestNode());
}

SampleBrokerApp::~SampleBrokerApp()
{
	shvInfo() << "Destroying SHV BROKER application object";
}

QString SampleBrokerApp::versionString() const
{
	return QCoreApplication::applicationVersion();
}

AppCliOptions *SampleBrokerApp::cliOptions()
{
	return dynamic_cast<AppCliOptions*>(Super::cliOptions());
}

shv::iotqt::rpc::Password SampleBrokerApp::password(const std::string &user)
{
	if(user == "foo") {
		shv::iotqt::rpc::Password ret;
		ret.password = "bar";
		ret.format = shv::iotqt::rpc::Password::Format::Plain;
		return ret;
	}
	if(user == "admin") {
		shv::iotqt::rpc::Password ret;
		ret.password = "19b9eab2dea2882d328caa6bc26b0b66c002813b";
		ret.format = shv::iotqt::rpc::Password::Format::Sha1;
		return ret;
	}
	return shv::iotqt::rpc::Password();
}

std::set<std::string> SampleBrokerApp::aclUserFlattenRoles(const std::string &user_name)
{
	std::set<std::string> ret;
	if(user_name == "foo")
		ret.insert("user");
	else if(user_name == "admin")
		ret.insert("poweruser");
	return ret;
}

cp::AclRole SampleBrokerApp::aclRole(const std::string &role_name)
{
	cp::AclRole ret;
	ret.name = role_name;
	return ret;
}

cp::AclRolePaths SampleBrokerApp::aclRolePaths(const std::string &role_name)
{
	cp::AclRolePaths ret;
	if(role_name == "user") {
		{
			cp::AccessGrant &grant = ret["**"];
			grant.type = cp::AccessGrant::Type::AccessLevel;
			//grant.role = cp::Rpc::ROLE_BROWSE;
			grant.accessLevel = cp::MetaMethod::AccessLevel::Browse;
		}
		{
			cp::AccessGrant &grant = ret["eyassrv/**"];
			grant.type = cp::AccessGrant::Type::Role;
			grant.role = cp::Rpc::ROLE_COMMAND;
		}
	}
	else if(role_name == "poweruser") {
		{
			cp::AccessGrant &grant = ret["**"];
			grant.type = cp::AccessGrant::Type::AccessLevel;
			//grant.role = cp::Rpc::ROLE_ADMIN;
			grant.accessLevel = cp::MetaMethod::AccessLevel::Admin;
		}
	}
	return ret;
}


