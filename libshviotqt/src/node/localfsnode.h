#pragma once

#include "shvnode.h"

#include <QDir>
#include <QMap>

namespace shv {
namespace iotqt {
namespace node {

class SHVIOTQT_DECL_EXPORT LocalFSNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	LocalFSNode(const QString &root_path, Super *parent = nullptr);

	size_t childCount(const std::string &shv_path = std::string()) override;
	std::string childName(size_t ix, const std::string &shv_path = std::string()) override;
	shv::chainpack::RpcValue call(const shv::chainpack::RpcValue &method_params, const std::string &shv_path = std::string()) override;
	//shv::chainpack::RpcValue ls(const shv::chainpack::RpcValue &methods_params, const std::string &shv_path = std::string()) override;

	//shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params, const std::string &shv_path = std::string()) override;
	size_t methodCount(const std::string &shv_path = std::string()) override;
	const shv::chainpack::MetaMethod* metaMethod(size_t ix, const std::string &shv_path = std::string()) override;
private:
	QFileInfo ndFileInfo(const std::string &path);
	chainpack::RpcValue ndSize(const std::string &path);
	chainpack::RpcValue ndRead(const std::string &path);
	/*
	chainpack::RpcValue ndLs(const std::string &path, const chainpack::RpcValue &methods_params);
	shv::chainpack::RpcValue ndCall(const std::string &path, const std::string &method, const shv::chainpack::RpcValue &params);
	//chainpack::RpcValue dir(const chainpack::RpcValue &methods_params) override;
	chainpack::RpcValue processRpcRequest(const chainpack::RpcRequest &rq) override;
	*/
private:
	QDir m_rootDir;
	QMap<QString, QStringList> m_dirCache;
};

} // namespace node
} // namespace iotqt
} // namespace shv

