#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace shv {
namespace broker {

class EtcAclNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	EtcAclNode(shv::iotqt::node::ShvNode *parent = nullptr);
	//~EtcAclNode() override;
};

class BrokerConfigFileNode : public shv::iotqt::node::RpcValueMapNode
{
	using Super = shv::iotqt::node::RpcValueMapNode;
public:
	BrokerConfigFileNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent = nullptr);
	//~BrokerConfigFileNode() override;

	//shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
protected:
	void loadValues() override;
	void saveValues() override;
};

class AclGrantsNode : public BrokerConfigFileNode
{
	using Super = BrokerConfigFileNode;
public:
	AclGrantsNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;

	bool addGrant(const shv::chainpack::RpcValue &params);
	bool editGrant(const shv::chainpack::RpcValue &params);
	bool delGrant(const shv::chainpack::RpcValue &params);
};

class AclUsersNode : public BrokerConfigFileNode
{
	using Super = BrokerConfigFileNode;
public:
	AclUsersNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;

	bool addUser(const shv::chainpack::RpcValue &params);
	bool delUser(const shv::chainpack::RpcValue &params);
};

class AclPathsNode : public BrokerConfigFileNode
{
	using Super = BrokerConfigFileNode;
public:
	AclPathsNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
	StringList childNames(const ShvNode::StringViewList &shv_path) override;

	bool setGrantPaths(const shv::chainpack::RpcValue &params);
	bool delGrantPaths(const shv::chainpack::RpcValue &params);
	shv::chainpack::RpcValue getGrantPaths(const shv::chainpack::RpcValue &params);

	//shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override {return Super::hasChildren(rewriteShvPath(shv_path));}
protected:
	shv::chainpack::RpcValue valueOnPath(const StringViewList &shv_path, bool throw_exc = true) override;
private:
	//StringViewList rewriteShvPath(const StringViewList &shv_path);
};

}}
