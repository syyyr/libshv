#include "shvnode.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/core/stringview.h>
#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace node {

ShvNode::ShvNode(ShvNode *parent)
	: QObject(parent)
{
	shvDebug() << __FUNCTION__ << this;
}

ShvNode *ShvNode::parentNode() const
{
	return qobject_cast<ShvNode*>(parent());
}

ShvNode *ShvNode::childNode(const ShvNode::String &name) const
{
	ShvNode *nd = findChild<ShvNode*>(QString::fromStdString(name), Qt::FindDirectChildrenOnly);
	return nd;
}

ShvNode *ShvNode::childNode(const shv::core::StringView &name) const
{
	return childNode(name.toString());
}

void ShvNode::setParentNode(ShvNode *parent)
{
	setParent(parent);
}

void ShvNode::setNodeId(ShvNode::String &&n)
{
	setObjectName(QString::fromStdString(n));
	shvDebug() << __FUNCTION__ << this << n;
	m_nodeId = std::move(n);
}

void ShvNode::setNodeId(const ShvNode::String &n)
{
	setObjectName(QString::fromStdString(n));
	shvDebug() << __FUNCTION__ << this << n;
	m_nodeId = n;
}

ShvNode::String ShvNode::shvPath() const
{
	String ret;
	const ShvNode *nd = this;
	while(nd) {
		ret = '/' + ret;
		if(!nd->isRootNode())
			ret = nd->nodeId() + ret;
		nd = nd->parentNode();
	}
	return ret;
}

/*
chainpack::RpcValue ShvNode::dir(chainpack::RpcValue meta_methods_params)
{
	Q_UNUSED(meta_methods_params)
	static cp::RpcValue::List methods{"get", "set"};
	return methods;
}
*/
ShvNode::StringList ShvNode::childNodeIds() const
{
	StringList ret;
	for(ShvNode *nd : findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly)) {
		shvDebug() << "child:" << nd << nd->nodeId();
		ret.push_back(nd->nodeId());
	}
	return ret;
}

shv::chainpack::RpcValue ShvNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	if(!rq.shvPath().empty())
		SHV_EXCEPTION("Subtree RPC request'" + shvPath() + "' on single node!");
	shv::chainpack::RpcValue ret = call(rq.method(), rq.params());
	return ret;
}

chainpack::RpcValue ShvNode::call(const std::string &method, const chainpack::RpcValue &params)
{
	shv::chainpack::RpcValue ret;
	if(method == cp::Rpc::METH_LS) {
		ret = ls(params);
	}
	else if(method == cp::Rpc::METH_DIR) {
		ret = dir(params);
	}
	else {
		SHV_EXCEPTION("Invalid method: " + method + " called for node: " + shvPath());
	}
	return ret;
}

chainpack::RpcValue ShvNode::ls(const chainpack::RpcValue &methods_params)
{
	const chainpack::RpcValue::List &mps = methods_params.toList();
	cp::RpcValue::List ret;
	for(const std::string &n : childNodeIds()) {
		ret.push_back(n);
		for(const chainpack::RpcValue &mp : mps) {
			std::string method = mp.isList()? mp.toList().value(0).toString(): mp.toString();
			chainpack::RpcValue params = mp.isList()? mp.toList().value(1): chainpack::RpcValue();
			try {
				ShvNode *nd = childNode(n);
				ret.push_back(nd->call(method, params));
			} catch (shv::core::Exception &) {
				ret.push_back(nullptr);
			}
		}
	}
	return ret;
}

chainpack::RpcValue ShvNode::dir(const chainpack::RpcValue &methods_params)
{
	Q_UNUSED(methods_params)
	//static cp::RpcValue::List ret{cp::Rpc::METH_GET, cp::Rpc::METH_SET};
	static cp::RpcValue::List ret{cp::Rpc::METH_DIR, cp::Rpc::METH_LS};
	return ret;
}

/*
shv::chainpack::RpcValue ShvNode::propertyValue(const String &property_name) const
{
	ShvNode *nd = childNode(property_name);
	if(nd)
		return nd->shvValue();
	return shv::chainpack::RpcValue();
}

bool ShvNode::setPropertyValue(const ShvNode::String &property_name, const shv::chainpack::RpcValue &val)
{
	if(setPropertyValue_helper(property_name, val)) {
		emit propertyValueChanged(property_name, val);
		return true;
	}
	return false;
}

bool ShvNode::setPropertyValue_helper(const ShvNode::String &property_name, const shv::chainpack::RpcValue &val)
{
	Q_UNUSED(property_name)
	Q_UNUSED(val)
	return false;
}
*/
}}}
