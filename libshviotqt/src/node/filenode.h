#pragma once

#include "shvnode.h"

namespace shv {
namespace iotqt {
namespace node {

class SHVIOTQT_DECL_EXPORT FileNode : public ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	static const char *M_HASH;
	static const char *M_SIZE;
	static const char *M_SIZE_COMPRESSED;
	static const char *M_READ;
	static const char *M_READ_COMPRESSED;

	static const std::vector<shv::chainpack::MetaMethod> meta_methods_file_base;

public:
	FileNode(const std::string &node_id, Super *parent = nullptr);

	chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params) override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

protected:
	virtual std::string fileName(const ShvNode::StringViewList &shv_path) const;
	virtual shv::chainpack::RpcValue size(const ShvNode::StringViewList &shv_path) const;
	virtual shv::chainpack::RpcValue readContent(const ShvNode::StringViewList &shv_path) const = 0;

private:
	shv::chainpack::RpcValue read(const ShvNode::StringViewList &shv_path) const;
	shv::chainpack::RpcValue readFileCompressed(const ShvNode::StringViewList &shv_path, const shv::chainpack::RpcValue &params) const;
};

}}}
