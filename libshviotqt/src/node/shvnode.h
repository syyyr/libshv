#pragma once

#include "../shviotqtglobal.h"

//#include <shv/chainpack/rpc.h>
#include <shv/chainpack/rpcvalue.h>

#include <QObject>

namespace shv { namespace chainpack { class MetaMethod; class RpcValue; class RpcMessage; class RpcRequest; }}
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
public:
	explicit ShvNode(ShvNode *parent = nullptr);

	//size_t childNodeCount() const {return propertyNames().size();}
	ShvNode* parentNode() const;
	virtual ShvNode* childNode(const String &name) const;
	//ShvNode* childNode(const core::StringView &name) const;
	virtual void setParentNode(ShvNode *parent);
	virtual String nodeId() const {return m_nodeId;}
	void setNodeId(String &&n);
	void setNodeId(const String &n);

	String shvPath() const;
	ShvRootNode* rootNode();

	virtual bool isRootNode() const {return false;}


	virtual void processRawData(const shv::chainpack::RpcValue::MetaData &meta, std::string &&data);
	virtual chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq);

	virtual shv::chainpack::RpcValue dir(const shv::chainpack::RpcValue &methods_params, const std::string &shv_path = std::string());
	virtual StringList methodNames(const std::string &shv_path = std::string());

	virtual shv::chainpack::RpcValue ls(const shv::chainpack::RpcValue &methods_params, const std::string &shv_path = std::string());
	virtual bool hasChildren(const std::string &shv_path = std::string());
	virtual shv::chainpack::RpcValue lsAttributes(unsigned attributes, const std::string &shv_path = std::string());
public:
	virtual size_t methodCount(const std::string &shv_path = std::string());
	virtual const shv::chainpack::MetaMethod* metaMethod(size_t ix, const std::string &shv_path = std::string());

	virtual StringList childNames(const std::string &shv_path = std::string());

	virtual shv::chainpack::RpcValue call(const std::string &method, const shv::chainpack::RpcValue &params, const std::string &shv_path = std::string());
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
