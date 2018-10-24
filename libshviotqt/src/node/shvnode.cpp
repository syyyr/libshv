#include "shvnode.h"

#include <shv/coreqt/log.h>

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpcdriver.h>
#include <shv/core/stringview.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace node {

ShvNode::ShvNode(ShvNode *parent)
	: QObject(parent)
{
	shvDebug() << __FUNCTION__ << this;
}

ShvNode::ShvNode(const std::string &node_id, ShvNode *parent)
	: ShvNode(parent)
{
	setNodeId(node_id);
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
	//SHV_EXCEPTION("Cannot find root node!");
	return nullptr;
}

void ShvNode::deleteDanglingPath()
{
	ShvNode *nd = this;
	ShvNode *dangling_nd = nullptr;
	while(!nd->isRootNode() && nd->childNames().empty()) {
		dangling_nd = nd;
		nd = nd->parentNode();
	}
	if(dangling_nd)
		delete dangling_nd;
}

void ShvNode::handleRawRpcRequest(cp::RpcValue::MetaData &&meta, std::string &&data)
{
	shvLogFuncFrame() << "node:" << nodeId() << "meta:" << meta.toStdString();
	const chainpack::RpcValue::String &method = cp::RpcMessage::method(meta).toString();
	const chainpack::RpcValue::String &shv_path_str = cp::RpcMessage::shvPath(meta).toString();
	core::StringView::StringViewList shv_path = core::StringView(shv_path_str).split('/');
	cp::RpcResponse resp = cp::RpcResponse::forRequest(meta);
	try {
		const chainpack::MetaMethod *mm = metaMethod(shv_path, method);
		if(mm) {
			std::string errmsg;
			cp::RpcMessage rpc_msg = cp::RpcDriver::composeRpcMessage(std::move(meta), data, &errmsg);
			if(!errmsg.empty())
				SHV_EXCEPTION(errmsg);

			cp::RpcRequest rq(rpc_msg);
			chainpack::RpcValue ret_val = processRpcRequest(rq);
			if(ret_val.isValid()) {
				resp.setResult(ret_val);
			}
		}
		else {
			if(!shv_path.empty()) {
				ShvNode *nd = childNode(shv_path.at(0).toString());
				if(nd) {
					std::string new_path = core::StringView::join(++shv_path.begin(), shv_path.end(), '/');
					//cp::RpcValue::MetaData meta2(meta);
					cp::RpcMessage::setShvPath(meta, new_path);
					nd->handleRawRpcRequest(std::move(meta), std::move(data));
					return;
				}
			}
		}
	}
	catch (std::exception &e) {
		shvError() << e.what();
		resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.what()));
	}
	if(resp.hasRetVal()) {
		ShvRootNode *root = rootNode();
		if(root) {
			root->emitSendRpcMesage(resp);
		}
	}
}

void ShvNode::handleRpcRequest(const chainpack::RpcRequest &rq)
{
	shvLogFuncFrame() << "node:" << nodeId();
	const chainpack::RpcValue::String &method = rq.method().toString();
	const chainpack::RpcValue::String &shv_path_str = rq.shvPath().toString();
	core::StringView::StringViewList shv_path = core::StringView(shv_path_str).split('/');
	cp::RpcResponse resp = cp::RpcResponse::forRequest(rq);
	try {
		const chainpack::MetaMethod *mm = metaMethod(shv_path, method);
		if(mm) {
			chainpack::RpcValue ret_val = processRpcRequest(rq);
			if(ret_val.isValid()) {
				resp.setResult(ret_val);
			}
		}
		else {
			if(!shv_path.empty()) {
				ShvNode *nd = childNode(shv_path.at(0).toString(), !shv::core::Exception::Throw);
				if(nd) {
					std::string new_path = core::StringView::join(++shv_path.begin(), shv_path.end(), '/');
					chainpack::RpcRequest rq2(rq);
					//cp::RpcValue::MetaData meta2(meta);
					rq2.setShvPath(new_path);
					nd->handleRpcRequest(rq2);
					return;
				}
				else {
					SHV_EXCEPTION("Method: '" + method + "' on path '" + shvPath() + '/' + shv_path_str + "' doesn't exist" + shvPath());
				}
			}
		}
	}
	catch (std::exception &e) {
		shvError() << e.what();
		resp.setError(cp::RpcResponse::Error::create(cp::RpcResponse::Error::MethodCallException, e.what()));
	}
	if(resp.hasRetVal()) {
		ShvRootNode *root = rootNode();
		if(root) {
			root->emitSendRpcMesage(resp);
		}
	}
}

chainpack::RpcValue ShvNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	core::StringView::StringViewList shv_path = core::StringView(rq.shvPath().toString()).split('/');
	const chainpack::RpcValue::String method = rq.method().toString();
	chainpack::RpcValue ret_val = callMethod(shv_path, method, rq.params());
	return ret_val;
}
/*
shv::chainpack::RpcValue ShvNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	if(!rq.shvPath().toString().empty())
		SHV_EXCEPTION("Invalid subpath: " + rq.shvPath().toCpon() + " method: " + rq.method().toCpon() + " called for node: " + shvPath());
	shv::chainpack::RpcValue ret = call(rq.method().toString(), rq.params());
	if(rq.requestId().toUInt() == 0)
		return cp::RpcValue(); // RPC calls with requestID == 0 does not expect response
	return ret;
}
*/

static std::string join_str(const ShvNode::StringList &sl, char sep)
{
	std::string ret;
	for(const std::string &s : sl) {
		if(ret.empty())
			ret = s;
		else
			ret += sep + s;
	}
	return ret;
}

QList<ShvNode *> ShvNode::ownChildren() const
{
	QList<ShvNode*> lst = findChildren<ShvNode*>(QString(), Qt::FindDirectChildrenOnly);
	qSort(lst);
	return lst;
}

ShvNode::StringList ShvNode::childNames(const StringViewList &shv_path)
{
	shvLogFuncFrame() << "node:" << nodeId() << "shv_path:" << StringView::join(shv_path, '/');
	ShvNode::StringList ret;
	if(shv_path.empty()) {
		for (ShvNode *nd : ownChildren()) {
			ret.push_back(nd->nodeId());
		}
	}
	else if(shv_path.size() == 1) {
		ShvNode *nd = childNode(shv_path.at(0).toString(), !shv::core::Exception::Throw);
		if(nd)
			ret = nd->childNames(StringViewList());
	}
	shvDebug() << "\tret:" << join_str(ret, '+');
	return ret;
}

chainpack::RpcValue ShvNode::hasChildren(const StringViewList &shv_path)
{
	return !childNames(shv_path).empty();
}

chainpack::RpcValue ShvNode::lsAttributes(const StringViewList &shv_path, unsigned attributes)
{
	shvLogFuncFrame() << "node:" << nodeId() << "attributes:" << attributes << "shv path:" << StringView::join(shv_path, '/');
	cp::RpcValue::List ret;
	if(shv_path.empty()) {
		if(attributes & (int)cp::LsAttribute::HasChildren)
			ret.push_back(hasChildren(shv_path));
	}
	else if(shv_path.size() == 1) {
		ShvNode *nd = childNode(shv_path.at(0).toString(), !shv::core::Exception::Throw);
		if(nd) {
			if(attributes & (int)cp::LsAttribute::HasChildren)
				ret.push_back(nd->hasChildren(StringViewList()));
		}
	}
	return ret;
}
/*
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
*/
chainpack::RpcValue ShvNode::dir(const StringViewList &shv_path, const chainpack::RpcValue &methods_params)
{
	cp::RpcValue::List ret;
	chainpack::RpcValueGenList params(methods_params);
	const std::string method = params.value(0).toString();
	unsigned attrs = params.value(1).toUInt();
	size_t cnt = methodCount(shv_path);
	for (size_t ix = 0; ix < cnt; ++ix) {
		const chainpack::MetaMethod *mm = metaMethod(shv_path, ix);
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

chainpack::RpcValue ShvNode::ls(const StringViewList &shv_path, const chainpack::RpcValue &methods_params)
{
	cp::RpcValue::List ret;
	chainpack::RpcValueGenList mpl(methods_params);
	const std::string child_name_pattern = mpl.value(0).toString();
	unsigned attrs = mpl.value(1).toUInt();
	for(const std::string &child_name : childNames(shv_path)) {
		if(child_name_pattern.empty() || child_name_pattern == child_name) {
			try {
				StringViewList ch_shv_path = shv_path;
				ch_shv_path.push_back(shv::core::StringView(child_name));
				cp::RpcValue::List attrs_result = lsAttributes(ch_shv_path, attrs).toList();
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

size_t ShvNode::methodCount(const StringViewList &shv_path)
{
	if(shv_path.empty())
		return meta_methods.size();
	return 0;
}

const chainpack::MetaMethod *ShvNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	if(shv_path.empty())
		return &(meta_methods.at(ix));
	return nullptr;
}

const chainpack::MetaMethod *ShvNode::metaMethod(const ShvNode::StringViewList &shv_path, const std::string &name)
{
	for (size_t i = 0; i < methodCount(shv_path); ++i) {
		const chainpack::MetaMethod *mm = metaMethod(shv_path, i);
		if(mm && name == mm->name())
			return mm;
	}
	return nullptr;
}
/*
ShvNode::StringList ShvNode::methodNames(const StringViewList &shv_path)
{
	ShvNode::StringList ret;
	size_t cnt = methodCount();
	for (size_t ix = 0; ix < cnt; ++ix) {
		ret.push_back(metaMethod(ix)->name());
	}
	return ret;
}
*/

chainpack::RpcValue ShvNode::callMethod(const ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(method == cp::Rpc::METH_DIR)
		return dir(shv_path, params);
	if(method == cp::Rpc::METH_LS)
		return ls(shv_path, params);

	SHV_EXCEPTION("Invalid method: " + method + " on path: " + core::StringView::join(shv_path, '/'));
}

void ShvRootNode::emitSendRpcMesage(const chainpack::RpcMessage &msg)
{
	if(msg.isResponse()) {
		cp::RpcResponse resp(msg);
		if(resp.requestId().toInt() == 0) // RPC calls with requestID == 0 does not expect response
			return;
	}
	emit sendRpcMesage(msg);
}

}}}
