#include "shvnode.h"

#include <shv/chainpack/metamethod.h>
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

ShvNode::ShvNode(ShvNode *parent)
	: QObject(parent)
{
	shvDebug() << __FUNCTION__ << this;
}

ShvNode *ShvNode::parentNode() const
{
	return qobject_cast<ShvNode*>(parent());
}

ShvNode *ShvNode::childNode(const ShvNode::String &name, bool throw_exc) const
{
	ShvNode *nd = findChild<ShvNode*>(QString::fromStdString(name), Qt::FindDirectChildrenOnly);
	if(throw_exc && !nd)
		SHV_EXCEPTION("Child node id: " + name + " doesn't exist, parent node: " + shvPath());
	return nd;
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
		if(!nd->isRootNode()) {
			if(!ret.empty())
				ret = '/' + ret;
			ret = nd->nodeId() + ret;
		}
		else {
			break;
		}
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
		resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.what()));
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
	if(!rq.shvPath().toString().empty())
		SHV_EXCEPTION("Invalid subpath: " + rq.shvPath().toCpon() + " method: " + rq.method().toCpon() + " called for node: " + shvPath());
	shv::chainpack::RpcValue ret = call(rq.method().toString(), rq.params());
	if(rq.requestId().toUInt() == 0)
		return cp::RpcValue(); // RPC calls with requestID == 0 does not expect response
	return ret;
}

ShvNode::StringList ShvNode::childNames()
{
	ShvNode::StringList ret;
	QList<ShvNode*> lst = findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly);
	for (ShvNode *nd : lst) {
		ret.push_back(nd->nodeId());
	}
	return ret;
}

chainpack::RpcValue ShvNode::hasChildren()
{
	return !childNames().empty();
}

chainpack::RpcValue ShvNode::lsAttributes(unsigned attributes)
{
	shvLogFuncFrame() << "attributes:" << attributes << "shv path:" << shvPath();
	cp::RpcValue::List ret;
	if(attributes & (int)cp::LsAttribute::HasChildren) {
		ret.push_back(hasChildren());
	}
	return ret;
}

chainpack::RpcValue ShvNode::call(const std::string &method, const chainpack::RpcValue &params)
{
	shvLogFuncFrame() << "method:" << method << "params:" << params.toCpon() << "shv path:" << shvPath();
	if(method == cp::Rpc::METH_LS) {
		return ls(params);
	}
	if(method == cp::Rpc::METH_DIR) {
		return dir(params);
	}
	SHV_EXCEPTION("Invalid method: " + method + " called for node: " + shvPath());
}

chainpack::RpcValue ShvNode::ls(const chainpack::RpcValue &methods_params)
{
	chainpack::RpcValueGenList mpl(methods_params);
	const std::string child_name_pattern = mpl.value(0).toString();
	unsigned attrs = mpl.value(1).toUInt();
	cp::RpcValue::List ret;
	for(const std::string &child_name : childNames()) {
		if(child_name_pattern.empty() || child_name_pattern == child_name) {
			//std::string path = shv_path.empty()? child_name: shv_path + '/' + child_name;
			try {
				cp::RpcValue::List attrs_result = childNode(child_name)->lsAttributes(attrs).toList();
				if(attrs_result.empty()) {
					ret.push_back(child_name);
				}
				else {
					attrs_result.insert(attrs_result.begin(), child_name);
					ret.push_back(attrs_result);
				}
			}
			catch (std::exception &) {
				ret.push_back(nullptr);
			}
		}
	}
	return ret;
}

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
};

size_t ShvNode::methodCount()
{
	return meta_methods.size();
}

const chainpack::MetaMethod *ShvNode::metaMethod(size_t ix)
{
	return &(meta_methods.at(ix));
}

ShvNode::StringList ShvNode::methodNames()
{
	ShvNode::StringList ret;
	size_t cnt = methodCount();
	for (size_t ix = 0; ix < cnt; ++ix) {
		ret.push_back(metaMethod(ix)->name());
	}
	return ret;
}

chainpack::RpcValue ShvNode::dir(const chainpack::RpcValue &methods_params)
{
	cp::RpcValue::List ret;
	chainpack::RpcValueGenList params(methods_params);
	const std::string method = params.value(0).toString();
	unsigned attrs = params.value(1).toUInt();
	size_t cnt = methodCount();
	for (size_t ix = 0; ix < cnt; ++ix) {
		const chainpack::MetaMethod *mm = metaMethod(ix);
		if(method.empty()) {
			ret.push_back(mm->attributes(attrs));
		}
		else if(method == mm->name()) {
				ret.push_back(mm->attributes(attrs));
				break;
		}
	}
	return ret;
}

}}}
