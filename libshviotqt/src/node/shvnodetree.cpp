#include "shvnodetree.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/stringview.h>
#include <shv/coreqt/log.h>

namespace shv {
namespace iotqt {
namespace node {

ShvNodeTree::ShvNodeTree(QObject *parent)
	: ShvNodeTree(new ShvRootNode(this), parent)
{
}

ShvNodeTree::ShvNodeTree(ShvRootNode *root, QObject *parent)
	: QObject(parent)
	, m_root(root)
{
	//m_root->setNodeId("<ROOT>");
}

ShvNodeTree::~ShvNodeTree()
{
	if(m_root && !m_root->parent())
		delete m_root;
}

ShvNode *ShvNodeTree::mkdir(const ShvNode::String &path)
{
	ShvNode::StringViewList lst = shv::core::StringView(path).split('/');
	return mkdir(lst);
}

ShvNode *ShvNodeTree::mkdir(const ShvNode::StringViewList &path)
{
	return mdcd(path, true, nullptr);
}

ShvNode *ShvNodeTree::cd(const ShvNode::String &path)
{
	std::string path_rest;
	ShvNode::StringViewList lst = shv::core::StringView(path).split('/');
	ShvNode *nd = mdcd(lst, false, &path_rest);
	if(path_rest.empty())
		return nd;
	return nullptr;
}

ShvNode *ShvNodeTree::cd(const ShvNode::String &path, ShvNode::String *path_rest)
{
	ShvNode::StringViewList lst = shv::core::StringView(path).split('/');
	//shvWarning() << path << "->" << shv::core::String::join(lst, '-');
	return mdcd(lst, false, path_rest);
}

ShvNode *ShvNodeTree::mdcd(const ShvNode::StringViewList &path, bool create_dirs, ShvNode::String *path_rest)
{
	ShvNode *ret = m_root;
	size_t ix;
	for (ix = 0; ix < path.size(); ++ix) {
		const shv::core::StringView &s = path[ix];
		ShvNode *nd2 = ret->childNode(s);
		if(nd2 == nullptr) {
			if(create_dirs) {
				ret = new ShvNode(ret);
				ret->setNodeId(path[ix].toString());
			}
			else {
				break;
			}
		}
		else {
			ret = nd2;
		}
	}
	if(path_rest) {
		auto path2 = ShvNode::StringViewList(path.begin() + ix, path.end());
		*path_rest = shv::core::String::join(path2, '/');
	}
	return ret;
}

bool ShvNodeTree::mount(const ShvNode::String &path, ShvNode *node)
{
	ShvNode::StringViewList lst = shv::core::StringView(path).split('/');
	if(lst.empty()) {
		shvError() << "Cannot mount node on empty path:" << path;
		return false;
	}
	shv::core::StringView last_id = lst.back();
	lst.pop_back();
	ShvNode *parent_nd = nullptr;
	if(lst.empty()) {
		parent_nd = m_root;
	}
	else {
		parent_nd = mkdir(lst);
	}
	ShvNode *ch = parent_nd->childNode(last_id);
	if(ch) {
		shvError() << "Node exist allready:" << path;
		return false;
	}
	node->setParentNode(parent_nd);
	node->setNodeId(last_id.toString());
	return true;
}

static std::string dump_node(ShvNode *parent, int indent)
{
	std::string ret;
	for(const std::string &pn : parent->childNodeIds()) {
		ShvNode *chnd = parent->childNode(pn);
		ret += '\n' + std::string(indent, '\t') + pn;// + ": " + parent->propertyValue(pn).toPrettyString();
		if(chnd) {
			ret += dump_node(chnd, ++indent);
		}
	}
	return ret;
}

std::string ShvNodeTree::dumpTree()
{
	return dump_node(m_root, 0);
}

}}}
