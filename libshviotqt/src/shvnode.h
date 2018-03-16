#pragma once

#include "shviotqtglobal.h"

//#include <shv/chainpack/rpcvalue.h>

#include <QObject>

namespace shv { namespace chainpack { class RpcValue; class RpcMessage; class RpcRequest; }}
namespace shv { namespace core { class StringView; }}

namespace shv {
namespace iotqt {

class SHVIOTQT_DECL_EXPORT ShvNode : public QObject
{
	Q_OBJECT
public:
	using StringList = std::vector<std::string>;
	using StringViewList = std::vector<shv::core::StringView>;
	using String = std::string;
public:
	explicit ShvNode(QObject *parent = nullptr);

	//size_t childNodeCount() const {return propertyNames().size();}
	ShvNode* parentNode() const;
	virtual ShvNode* childNode(const String &name) const;
	virtual ShvNode* childNode(const core::StringView &name) const;
	virtual void setParentNode(ShvNode *parent);
	virtual String nodeId() const {return m_nodeId;}
	void setNodeId(String &&n);
	void setNodeId(const String &n);

	String shvPath() const;

	//virtual shv::chainpack::RpcValue dir(shv::chainpack::RpcValue meta_methods_params);
	virtual shv::chainpack::RpcValue ls(shv::chainpack::RpcValue methods_params);

	virtual bool isRootNode() const {return false;}

	virtual shv::chainpack::RpcValue shvValue() const;

	virtual StringList childNodeIds() const;
	//virtual shv::chainpack::RpcValue propertyValue(const String &property_name) const;
	//virtual bool setPropertyValue(const String &property_name, const shv::chainpack::RpcValue &val);
	//Q_SIGNAL void propertyValueChanged(const String &property_name, const shv::chainpack::RpcValue &new_val);
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
	explicit ShvRootNode(QObject *parent = nullptr) : Super(parent) {}

	bool isRootNode() const override {return true;}
};

}}
