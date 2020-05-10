#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace shv {
namespace broker {

class AclAccessRule;

class EtcAclNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	EtcAclNode(shv::iotqt::node::ShvNode *parent = nullptr);
};

class BrokerAclNode : public shv::iotqt::node::MethodsTableNode
{
	using Super = shv::iotqt::node::MethodsTableNode;
public:
	BrokerAclNode(const std::string &config_name, shv::iotqt::node::ShvNode *parent = nullptr);
protected:
	virtual std::vector<shv::chainpack::MetaMethod> *metaMethodsForPath(const StringViewList &shv_path);
	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	std::string saveConfigFile(const std::string &file_name, const shv::chainpack::RpcValue val);
};

class MountsAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	MountsAclNode(shv::iotqt::node::ShvNode *parent = nullptr);

protected:
	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
};

class RolesAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	RolesAclNode(shv::iotqt::node::ShvNode *parent = nullptr);

protected:
	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
};

class UsersAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	UsersAclNode(shv::iotqt::node::ShvNode *parent = nullptr);
protected:
	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
};

class AccessAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	AccessAclNode(shv::iotqt::node::ShvNode *parent = nullptr);

protected:
	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	std::vector<shv::chainpack::MetaMethod> *metaMethodsForPath(const StringViewList &shv_path) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
private:
	std::string ruleKey(unsigned rule_ix, unsigned rules_cnt, const shv::broker::AclAccessRule &rule) const;
	static constexpr auto InvalidIndex = std::numeric_limits<unsigned>::max();
	static unsigned keyToRuleIndex(const std::string &key);
};

}}
