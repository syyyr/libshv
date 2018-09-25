#pragma once

#include "shvnode.h"

#include <QDir>
#include <QMap>

namespace shv {
namespace iotqt {
namespace node {

class SHVIOTQT_DECL_EXPORT LocalFSNode : public ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	LocalFSNode(const QString &root_path, Super *parent = nullptr);

	chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;

	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	bool hasChildren(const StringViewList &shv_path) override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
private:
	QFileInfo ndFileInfo(const QString &path);
	chainpack::RpcValue ndSize(const QString &path);
	chainpack::RpcValue ndRead(const QString &path);
private:
	QDir m_rootDir;
};

} // namespace node
} // namespace iotqt
} // namespace shv

