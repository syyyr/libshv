#pragma once

#include "shvnode.h"

#include <shv/core/string.h>

#include <QObject>

namespace shv {
namespace iotqt {
namespace node {

class SHVIOTQT_DECL_EXPORT ShvNodeTree : public QObject
{
	Q_OBJECT
public:
	explicit ShvNodeTree(QObject *parent = nullptr);
	explicit ShvNodeTree(ShvRootNode *root, QObject *parent = nullptr);
	~ShvNodeTree() override;

	ShvRootNode* root() const {return m_root;}

	ShvNode* mkdir(const ShvNode::String &path);
	ShvNode* mkdir(const ShvNode::StringViewList &path);
	ShvNode* cd(const ShvNode::String &path);
	ShvNode* cd(const ShvNode::String &path, ShvNode::String *path_rest);
	bool mount(const ShvNode::String &path, ShvNode *node);

	std::string dumpTree();
protected:
	ShvNode* mdcd(const ShvNode::StringViewList &path, bool create_dirs, ShvNode::String *path_rest);
protected:
	//std::map<std::string, ShvNode*> m_root;
	ShvRootNode* m_root = nullptr;
};

}}}
