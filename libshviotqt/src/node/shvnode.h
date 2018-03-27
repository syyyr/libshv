#pragma once

#include "../shviotqtglobal.h"

//#include <shv/chainpack/rpcvalue.h>

#include <QObject>

namespace shv { namespace chainpack { class RpcValue; class RpcMessage; class RpcRequest; }}
namespace shv { namespace core { class StringView; }}

namespace shv {
namespace iotqt {
namespace node {

class SHVIOTQT_DECL_EXPORT ShvNode : public QObject
{
	Q_OBJECT
public:
	using StringList = std::vector<std::string>;
	using StringViewList = std::vector<shv::core::StringView>;
	using String = std::string;
public:
	explicit ShvNode(ShvNode *parent = nullptr);

	//size_t childNodeCount() const {return propertyNames().size();}
	ShvNode* parentNode() const;
	virtual ShvNode* childNode(const String &name) const;
	virtual ShvNode* childNode(const core::StringView &name) const;
	virtual void setParentNode(ShvNode *parent);
	virtual String nodeId() const {return m_nodeId;}
	void setNodeId(String &&n);
	void setNodeId(const String &n);

	String shvPath() const;

	virtual shv::chainpack::RpcValue ls(const shv::chainpack::RpcValue &methods_params);
	virtual shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params);
	virtual shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params);

	virtual bool isRootNode() const {return false;}

	virtual StringList childNodeIds() const;
	virtual chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq);
protected:
	//virtual bool setPropertyValue_helper(const String &property_name, const shv::chainpack::RpcValue &val);
private:
	String m_nodeId;
};

class SHVIOTQT_DECL_EXPORT ShvRootNode : public ShvNode
{
	using Super = ShvNode;
public:
	explicit ShvRootNode() : Super(nullptr) {}

	bool isRootNode() const override {return true;}
};

}}}
