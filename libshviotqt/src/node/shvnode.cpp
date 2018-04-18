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
	shv::chainpack::RpcValue ret = call(rq.method().toString(), rq.params(), rq.shvPath().toString());
	if(rq.requestId().toUInt() == 0)
		return cp::RpcValue(); // RPC calls with requestID == 0 does not expect response
	return ret;
}
/*
size_t ShvNode::childCount(const std::string &shv_path)
{
	if(!shv_path.empty())
		return childNode(shv_path)->childCount();
	QList<ShvNode*> lst = findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly);
	return lst.size();
}

std::string ShvNode::childName(size_t ix, const std::string &shv_path)
{
	if(!shv_path.empty())
		return childNode(shv_path)->childName(ix);
	QList<ShvNode*> lst = findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly);
	return lst[ix]->nodeId();
}
*/
ShvNode::StringList ShvNode::childNames(const std::string &shv_path)
{
	if(shv_path.empty()) {
		ShvNode::StringList ret;
		QList<ShvNode*> lst = findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly);
		for (ShvNode *nd : lst) {
			ret.push_back(nd->nodeId());
		}
		return ret;
	}
	return childNode(shv_path)->childNames();
}

bool ShvNode::hasChildren(const std::string &shv_path)
{
	if(shv_path.empty()) {
		//shvWarning() << this->metaObject()->className() << "ShvNode::hasChildren path:" << shvPath() << "ret:" << cp::RpcValue(childNames()).toCpon();
		return !childNames().empty();
	}
	return childNode(shv_path)->hasChildren();
}

chainpack::RpcValue ShvNode::lsAttributes(unsigned attributes, const std::string &shv_path)
{
	//if(!shv_path.empty())
	//	SHV_EXCEPTION("ShvNode::call - subtree RPC request'" + shvPath() + "' on single node, method: " + __FUNCTION__ + " shv path: " + shv_path);
	cp::RpcValue::List ret;
	if(attributes & (int)cp::LsAttribute::HasChildren) {
		//std::string path = shv_path.empty()? node_id: shv_path + '/' + node_id;
		ret.push_back(hasChildren(shv_path));
		//ret.push_back(childNode(node_id)->hasChildren());
	}
	//shvWarning() << "lsAttributes path:" << shv_path << node_id << "ret:" << cp::RpcValue(ret).toCpon();
	return ret;
}

chainpack::RpcValue ShvNode::call(const std::string &method, const chainpack::RpcValue &params, const std::string &shv_path)
{
	//if(!shv_path.empty())
	//	SHV_EXCEPTION("ShvNode::call - subtree RPC request'" + shvPath() + "' on single node, method: " + method + " shv path: " + shv_path);
	if(method == cp::Rpc::METH_LS) {
		return ls(params, shv_path);
	}
	if(method == cp::Rpc::METH_DIR) {
		return dir(params, shv_path);
	}
	SHV_EXCEPTION("Invalid method: " + method + " called for node: " + shvPath());
}

chainpack::RpcValue ShvNode::ls(const chainpack::RpcValue &methods_params, const std::string &shv_path)
{
	//if(!shv_path.empty())
	//	return childNode(shv_path)->ls(methods_params);
	chainpack::RpcValueGenList mpl(methods_params);
	const std::string child_name_pattern = mpl.value(0).toString();
	unsigned attrs = mpl.value(1).toUInt();
	cp::RpcValue::List ret;
	for(const std::string &child_name : childNames(shv_path)) {
		if(child_name_pattern.empty() || child_name_pattern == child_name) {
			std::string path = shv_path.empty()? child_name: shv_path + '/' + child_name;
			try {
				cp::RpcValue::List attrs_result = lsAttributes(attrs, path).toList();
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

size_t ShvNode::methodCount(const std::string &shv_path)
{
	if(!shv_path.empty())
		return childNode(shv_path)->methodCount();
	return meta_methods.size();
}

const chainpack::MetaMethod *ShvNode::metaMethod(size_t ix, const std::string &shv_path)
{
	if(!shv_path.empty())
		return childNode(shv_path)->metaMethod(ix);
	return &(meta_methods.at(ix));
}

ShvNode::StringList ShvNode::methodNames(const std::string &shv_path)
{
	ShvNode::StringList ret;
	size_t cnt = methodCount(shv_path);
	for (size_t ix = 0; ix < cnt; ++ix) {
		ret.push_back(metaMethod(ix, shv_path)->name());
	}
	return ret;
}

chainpack::RpcValue ShvNode::dir(const chainpack::RpcValue &methods_params, const std::string &shv_path)
{
	cp::RpcValue::List ret;
	chainpack::RpcValueGenList params(methods_params);
	const std::string method = params.value(0).toString();
	unsigned attrs = params.value(1).toUInt();
	size_t cnt = methodCount(shv_path);
	for (size_t ix = 0; ix < cnt; ++ix) {
		const chainpack::MetaMethod *mm = metaMethod(ix, shv_path);
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
