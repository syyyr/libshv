#pragma once

#include "shvnode.h"

namespace shv {
namespace iotqt {
namespace node {

class SHVIOTQT_DECL_EXPORT ShvTreeNode : public ShvNode
{
	Q_OBJECT
	using Super = ShvNode;
public:
	explicit ShvTreeNode(ShvNode *parent) : Super(parent) {}

	//virtual void processRawData(const shv::chainpack::RpcValue::MetaData &meta, std::string &&data);
	virtual chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq) override;

	/*
	shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params) override {return dir2(methods_params, std::string());}
	StringList methodNames() override {return methodNames2(std::string());}

	StringList childNames() override {return childNames2(std::string());}
	chainpack::RpcValue lsAttributes(unsigned attributes) override {return lsAttributes2(attributes, std::string());}

	size_t methodCount() override {return methodCount2(std::string());}
	const shv::chainpack::MetaMethod* metaMethod(size_t ix) override {return metaMethod2(ix, std::string());}
	*/
public:
	virtual shv::chainpack::RpcValue dir2(const shv::chainpack::RpcValue &methods_params, const std::string &shv_path);
	virtual StringList methodNames2(const std::string &shv_path);

	virtual shv::chainpack::RpcValue ls2(const shv::chainpack::RpcValue &methods_params, const std::string &shv_path);
	virtual shv::chainpack::RpcValue hasChildren2(const std::string &shv_path);
	virtual shv::chainpack::RpcValue lsAttributes2(unsigned attributes, const std::string &shv_path);
	virtual shv::chainpack::RpcValue call2(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path);
public:
	virtual size_t methodCount2(const std::string &shv_path) = 0;
	virtual const shv::chainpack::MetaMethod* metaMethod2(size_t ix, const std::string &shv_path) = 0;

	virtual StringList childNames2(const std::string &shv_path) = 0;
};

} // namespace node
} // namespace iotqt
} // namespace shv
