#include "samplebrokerapp.h"
#include "aclmanager.h"

#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/log.h>
#include <shv/broker/aclrolepaths.h>

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
	auto *m = new AclManager(this);
	setAclManager(m);
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


