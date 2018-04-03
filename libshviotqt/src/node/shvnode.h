#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <QObject>

namespace shv { namespace chainpack { class RpcValue; class RpcMessage; class RpcRequest; }}
namespace shv { namespace core { class StringView; }}

namespace shv {
namespace iotqt {
namespace node {

class ShvRootNode;

class SHVIOTQT_DECL_EXPORT ShvNode : public QObject
{
	Q_OBJECT
public:
	using StringList = std::vector<std::string>;
	using StringViewList = std::vector<shv::core::StringView>;
	using String = std::string;

	class SHVIOTQT_DECL_EXPORT MethParams
	{
	public:
		MethParams() {}
		MethParams(const chainpack::RpcValue &method_params) : m_mp(method_params) {}
		std::string method() const;
		chainpack::RpcValue params() const;
	private:
		chainpack::RpcValue m_mp;
	};
	class SHVIOTQT_DECL_EXPORT MethParamsList : public std::vector<MethParams>
	{
	public:
		MethParamsList(const chainpack::RpcValue &method_params);
		MethParams value(size_t ix) const {return (ix < size())? operator [](ix): MethParams();}
	};
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
	ShvRootNode* rootNode();

	virtual shv::chainpack::RpcValue ls(const shv::chainpack::RpcValue &methods_params);
	virtual shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params);
	virtual shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params);

	virtual bool isRootNode() const {return false;}

	virtual StringList childNodeIds() const;

	virtual void processRawData(const shv::chainpack::RpcValue::MetaData &meta, std::string &&data);
	virtual chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq);
protected:
	//virtual bool setPropertyValue_helper(const String &property_name, const shv::chainpack::RpcValue &val);
private:
	String m_nodeId;
};

class SHVIOTQT_DECL_EXPORT ShvRootNode : public ShvNode
{
	Q_OBJECT
	using Super = ShvNode;
public:
	explicit ShvRootNode(QObject *parent) : Super() {setParent(parent);}

	bool isRootNode() const override {return true;}

	Q_SIGNAL void sendRpcMesage(const shv::chainpack::RpcMessage &msg);
	void emitSendRpcMesage(const shv::chainpack::RpcMessage &msg) {emit sendRpcMesage(msg);}
};

}}}
