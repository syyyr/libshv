#pragma once

#include "shvnode.h"

#include <QDir>

namespace shv {
namespace iotqt {
namespace node {

class SHVIOTQT_DECL_EXPORT LocalFSNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	LocalFSNode(const QString &root_path, Super *parent = nullptr);
private:
	QFileInfo ndFileInfo(const std::string &path);
	chainpack::RpcValue ndLs(const std::string &path, const chainpack::RpcValue &methods_params);
	chainpack::RpcValue ndSize(const std::string &path);
	chainpack::RpcValue ndRead(const std::string &path);
	//chainpack::RpcValue dir(const chainpack::RpcValue &methods_params) override;
	chainpack::RpcValue processRpcRequest(const chainpack::RpcRequest &rq) override;
private:
	QDir m_rootDir;
};

} // namespace node
} // namespace iotqt
} // namespace shv

