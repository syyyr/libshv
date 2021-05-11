#include "samplebrokerapp.h"
#include "aclmanager.h"

#include <shv/iotqt/node/shvnodetree.h>
#include <shv/coreqt/log.h>
#include <shv/iotqt/acl/aclroleaccessrules.h>

#include <QTimer>

namespace cp = shv::chainpack;

static const char METH_RESET[] = "reset";

class TestNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	TestNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(std::string(), &m_metaMethods, parent)
		, m_metaMethods{
			{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
			{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
		}
	{ }

private:
	std::vector<cp::MetaMethod> m_metaMethods;
};

class PropertyNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	PropertyNode(const std::string &prop_name, shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(prop_name, &m_metaMethods, parent)
		, m_metaMethods{
			{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
			{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Browse},
			{cp::Rpc::METH_GET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsGetter, cp::MetaMethod::AccessLevel::Read},
			{cp::Rpc::METH_SET, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSetter, cp::MetaMethod::AccessLevel::Write},
			{cp::Rpc::SIG_VAL_CHANGED, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::IsSignal, cp::MetaMethod::AccessLevel::Read},
		}
	{
	}

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override
	{
		if(shv_path.empty()) {
			if(method == cp::Rpc::METH_GET) {
				return getValue();
			}
			if(method == cp::Rpc::METH_SET) {
				setValue(params);
				return true;
			}
		}
		return Super::callMethod(shv_path, method, params);
	}

	cp::RpcValue getValue() const {return m_value;}
	void setValue(const cp::RpcValue &val)
	{
		if(val == m_value)
			return;
		m_value = val;
		cp::RpcSignal sig;
		sig.setMethod(cp::Rpc::SIG_VAL_CHANGED);
		sig.setShvPath(shvPath());
		sig.setParams(m_value);
		emitSendRpcMessage(sig);
	}
protected:
	std::vector<cp::MetaMethod> m_metaMethods;
	cp::RpcValue m_value;
};

class UptimeNode : public PropertyNode
{
	using Super = PropertyNode;
public:
	UptimeNode(shv::iotqt::node::ShvNode *parent = nullptr)
		: Super("uptime", parent)
	{
		m_metaMethods.push_back({METH_RESET, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::MetaMethod::AccessLevel::Command});

		auto *t = new QTimer(this);
		connect(t, &QTimer::timeout, this, &UptimeNode::incUptime);
		t->start(1000);
	}

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override
	{
		if(shv_path.empty()) {
			if(method == METH_RESET) {
				setValue(0);
				return true;
			}
		}
		return Super::callMethod(shv_path, method, params);
	}
private:
	void incUptime()
	{
		setValue(getValue().toInt() + 1);
	}
};

SampleBrokerApp::SampleBrokerApp(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv, cli_opts)
{
	auto *m = new AclManager(this);
	setAclManager(m);
	auto nd1 = new TestNode();
	m_nodesTree->mount("test", nd1);
	new UptimeNode(nd1);
	{
		auto *nd = new PropertyNode("someText", nd1);
		nd->setValue("Some text");
	}
	{
		auto *nd = new PropertyNode("someInt", nd1);
		nd->setValue(42);
	}

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


