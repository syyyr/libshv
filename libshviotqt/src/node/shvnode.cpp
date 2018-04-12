#include "shvnode.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpcdriver.h>
#include <shv/core/stringview.h>
#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace node {

std::string ShvNode::MethParams::method() const
{
	return m_mp.isList()? m_mp.toList().value(0).toString(): m_mp.toString();
}

chainpack::RpcValue ShvNode::MethParams::params() const
{
	return m_mp.isList()? m_mp.toList().value(1): chainpack::RpcValue();
}

ShvNode::MethParamsList::MethParamsList(const chainpack::RpcValue &method_params)
{
	for(const auto &v : method_params.toList())
		push_back(MethParams(v));
}

ShvNode::DirMethParamsList::DirMethParamsList(const chainpack::RpcValue &method_params)
{
	if(method_params.isString()) {
		m_method = method_params.toString();
	}
	else if(method_params.isList()) {
		const chainpack::RpcValue::List &lst = method_params.toList();
		m_method = lst.value(0).toString();
		for (size_t i = 1; i < lst.size(); ++i) {
			push_back(MethParams(lst[i]));
		}
	}
}

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
/*
ShvNode *ShvNode::childNode(const shv::core::StringView &name) const
{
	return childNode(name.toString());
}
*/
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

ShvRootNode *ShvNode::rootNode()
{
	ShvNode *nd = this;
	while(nd) {
		if(nd->isRootNode())
			return qobject_cast<ShvRootNode*>(nd);
		nd = nd->parentNode();
	}
	SHV_EXCEPTION("Cannot find root node!");
	return nullptr;
}

/*
chainpack::RpcValue ShvNode::dir(chainpack::RpcValue meta_methods_params)
{
	Q_UNUSED(meta_methods_params)
	static cp::RpcValue::List methods{"get", "set"};
	return methods;
}
*/
void ShvNode::processRawData(const chainpack::RpcValue::MetaData &meta, std::string &&data)
{
	shvLogFuncFrame() << meta.toStdString();
	std::string errmsg;
	cp::RpcMessage rpc_msg = cp::RpcDriver::composeRpcMessage(cp::RpcValue::MetaData(meta), data, &errmsg);
	if(!errmsg.empty()) {
		shvError() << errmsg;
		return;
	}
	cp::RpcRequest rq(rpc_msg);
	cp::RpcResponse resp = cp::RpcResponse::forRequest(rq.metaData());
	bool response_deffered = false;
	try {
		cp::RpcValue result = processRpcRequest(rq);
		shvDebug() << result.toPrettyString();
		if(result.isValid())
			resp.setResult(result);
		else
			response_deffered = true;
	}
	catch (std::exception &e) {
		shvError() << e.what();
		resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodInvocationException, e.what()));
	}
	if(!response_deffered) {
		ShvRootNode *root = rootNode();
		if(root) {
			root->emitSendRpcMesage(resp);
		}
	}
}

shv::chainpack::RpcValue ShvNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	//if(!rq.shvPath().toString().empty())
	//	SHV_EXCEPTION("Subtree RPC request'" + shvPath() + "' on single node!");
	shv::chainpack::RpcValue ret = call(rq.shvPath().toString(), rq.method().toString(), rq.params());
	if(rq.requestId().toUInt() == 0)
		return cp::RpcValue(); // RPC calls with requestID == 0 does not expect response
	return ret;
}

ShvNode::StringList ShvNode::childNodeIds(const std::string &shv_path) const
{
	if(!shv_path.empty())
		SHV_EXCEPTION("Subtree RPC request'" + shv_path + "' on single node!");
	StringList ret;
	for(ShvNode *nd : findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly)) {
		shvDebug() << "child:" << nd << nd->nodeId();
		ret.push_back(nd->nodeId());
	}
	return ret;
}

chainpack::RpcValue ShvNode::call(const std::string &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(!shv_path.empty())
		SHV_EXCEPTION("Subtree RPC request'" + shvPath() + "' on single node!");
	if(method == cp::Rpc::METH_LS) {
		return ls(params);
	}
	if(method == cp::Rpc::METH_DIR) {
		return dir(params);
	}
	{
		SHV_EXCEPTION("Invalid method: " + method + " called for node: " + shvPath());
	}
}

chainpack::RpcValue ShvNode::callChild(const std::string &shv_path, const std::string &node_id, const std::string &method, const chainpack::RpcValue &params)
{
	if(!shv_path.empty())
		SHV_EXCEPTION("Subtree RPC request'" + shvPath() + "' on single node!");
	ShvNode *nd = childNode(node_id);
	return nd->call(method, params);
}

chainpack::RpcValue ShvNode::ls(const std::string &shv_path, const chainpack::RpcValue &methods_params)
{
	MethParamsList mpl(methods_params);
	cp::RpcValue::List ret;
	for(const std::string &n : childNodeIds(shv_path)) {
		if(mpl.empty()) {
			ret.push_back(n);
		}
		else {
			cp::RpcValue::List ret1;
			ret1.push_back(n);
			for(const MethParams &mp : mpl) {
				try {
					ret1.push_back(callChild(shv_path, n, mp.method(), mp.params()));
				} catch (shv::core::Exception &) {
					ret1.push_back(nullptr);
				}
			}
			ret.push_back(ret1);
		}
	}
	return ret;
}

static const ShvNode::StringList& _methodNames()
{
	static ShvNode::StringList lst{cp::Rpc::METH_DIR};
	return lst;
}

ShvNode::StringList ShvNode::methodNames()
{
	return _methodNames();
}

chainpack::RpcValue ShvNode::dirMethod(const std::string &shv_path, const std::string &method, const chainpack::RpcValue &methods_params)
{
	Q_UNUSED(methods_params);
	if(!shv_path.empty())
		SHV_EXCEPTION("Subtree RPC request'" + shvPath() + "' on single node!");
	const StringList &mn = _methodNames();
	if(std::find(mn.begin(), mn.end(), method) == mn.end())
		return nullptr;
	return method;
}

chainpack::RpcValue ShvNode::dir(const std::string &shv_path, const chainpack::RpcValue &methods_params)
{
	//MethParamsList mpl(methods_params);
	if(!shv_path.empty())
		SHV_EXCEPTION("Subtree RPC request'" + shvPath() + "' on single node!");
	cp::RpcValue::List ret;
	if(method.empty()) {
		for(const std::string &m : methodNames()) {
			ret.push_back(dirMethod(shv_path, m, methods_params));
		}
	}
	else {
		ret.push_back(dirMethod(shv_path, method, methods_params));
	}
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
