#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace shv {
namespace broker {

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
};

class FstabAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	FstabAclNode(shv::iotqt::node::ShvNode *parent = nullptr);

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

class PathsAclNode : public BrokerAclNode
{
	using Super = BrokerAclNode;
public:
	PathsAclNode(shv::iotqt::node::ShvNode *parent = nullptr);

protected:
	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	std::vector<shv::chainpack::MetaMethod> *metaMethodsForPath(const StringViewList &shv_path) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;
};

}}
