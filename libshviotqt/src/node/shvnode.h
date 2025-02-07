#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/metamethod.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/rpcmessage.h>
#include <shv/core/stringview.h>
#include <shv/core/utils.h>

#include <QObject>
#include <QMetaProperty>

#include <cstddef>

//namespace shv { namespace chainpack { class MetaMethod; }}
//namespace shv { namespace chainpack { class MetaMethod; class RpcValue; class RpcMessage; class RpcRequest; }}
namespace shv { namespace core { namespace utils { class ShvPath; class ShvJournalEntry; }}}


namespace shv {
namespace iotqt {

namespace utils { class ShvPath; }

namespace node {

class ShvRootNode;

class SHVIOTQT_DECL_EXPORT ShvNode : public QObject
{
	Q_OBJECT
public:
	using String = std::string;
	using StringList = std::vector<String>;
	using StringView = shv::core::StringView;
	using StringViewList = shv::core::StringViewList;
public:
	static std::string ADD_LOCAL_TO_LS_RESULT_HACK_META_KEY;
	static std::string LOCAL_NODE_HACK;
public:
	explicit ShvNode(ShvNode *parent);
	explicit ShvNode(const std::string &node_id, ShvNode *parent = nullptr);
	~ShvNode() override;

	//size_t childNodeCount() const {return propertyNames().size();}
	ShvNode* parentNode() const;
	QList<ShvNode*> ownChildren() const;
	virtual ShvNode* childNode(const String &name, bool throw_exc = true) const;
	//ShvNode* childNode(const core::StringView &name) const;
	virtual void setParentNode(ShvNode *parent);
	virtual String nodeId() const {return m_nodeId;}
	void setNodeId(String &&n);
	void setNodeId(const String &n);

	shv::core::utils::ShvPath shvPath() const;
	//static StringViewList splitShvPath(const std::string &shv_path) { return StringView{shv_path}.split(SHV_PATH_DELIM, SHV_PATH_QUOTE); }
	//static String joinShvPath(const StringViewList &shv_path);

	ShvNode* rootNode();
	virtual void emitSendRpcMessage(const shv::chainpack::RpcMessage &msg);
	void emitLogUserCommand(const shv::core::utils::ShvJournalEntry &e);

	void setSortedChildren(bool b) {m_isSortedChildren = b;}

	void deleteIfEmptyWithParents();

	bool isRootNode() const {return m_isRootNode;}

	virtual void handleRawRpcRequest(chainpack::RpcValue::MetaData &&meta, std::string &&data);
	virtual void handleRpcRequest(const chainpack::RpcRequest &rq);
	virtual chainpack::RpcValue handleRpcRequestImpl(const chainpack::RpcRequest &rq);
	virtual chainpack::RpcValue processRpcRequest(const shv::chainpack::RpcRequest &rq);

	virtual shv::chainpack::RpcValue dir(const StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);

	virtual shv::chainpack::RpcValue ls(const StringViewList &shv_path, const shv::chainpack::RpcValue &methods_params);
	// returns null if does not know
	virtual chainpack::RpcValue hasChildren(const StringViewList &shv_path);
	virtual shv::chainpack::RpcValue lsAttributes(const StringViewList &shv_path, unsigned attributes);

	static int basicGrantToAccessLevel(const shv::chainpack::RpcValue &acces_grant);
	virtual int grantToAccessLevel(const shv::chainpack::RpcValue &acces_grant) const;

	void treeWalk(std::function<void (ShvNode *parent_nd, const StringViewList &shv_path)> callback) { treeWalk_helper(callback, this, {}); }
private:
	static void treeWalk_helper(std::function<void (ShvNode *parent_nd, const StringViewList &shv_path)> callback, ShvNode *parent_nd, const StringViewList &shv_path);
public:
	virtual size_t methodCount(const StringViewList &shv_path);
	virtual const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix);
	virtual const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, const std::string &name);

	virtual StringList childNames(const StringViewList &shv_path);
	StringList childNames() {return childNames(StringViewList());}

	virtual shv::chainpack::RpcValue callMethodRq(const chainpack::RpcRequest &rq);
	virtual shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id);
public:
	Q_SIGNAL void sendRpcMessage(const shv::chainpack::RpcMessage &msg);
	Q_SIGNAL void logUserCommand(const shv::core::utils::ShvJournalEntry &e);
protected:
	bool m_isRootNode = false;
private:
	String m_nodeId;
	bool m_isSortedChildren = true;
};

/// helper class to save lines when creating root node
/// any ShvNode descendant with m_isRootNode = true may be RootNode
class SHVIOTQT_DECL_EXPORT ShvRootNode : public ShvNode
{
	using Super = ShvNode;
public:
	explicit ShvRootNode(QObject *parent);
	~ShvRootNode() override;
};

class SHVIOTQT_DECL_EXPORT MethodsTableNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	//explicit MethodsTableNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent = nullptr)
	//	: Super(node_id, parent) {}
	explicit MethodsTableNode(const std::string &node_id, const std::vector<shv::chainpack::MetaMethod> *methods, shv::iotqt::node::ShvNode *parent = nullptr)
		: Super(node_id, parent), m_methods(methods) {}

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;
protected:
	const std::vector<shv::chainpack::MetaMethod> *m_methods = nullptr;
};


class SHVIOTQT_DECL_EXPORT RpcValueMapNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
	using Super = shv::iotqt::node::ShvNode;

	SHV_FIELD_BOOL_IMPL(r, R, eadOnly)
public:
	static const char *M_LOAD;
	static const char *M_SAVE;
	static const char *M_COMMIT;
public:
	RpcValueMapNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent = nullptr);
	RpcValueMapNode(const std::string &node_id, const shv::chainpack::RpcValue &values, shv::iotqt::node::ShvNode *parent = nullptr);
	//~RpcValueMapNode() override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	StringList childNames(const ShvNode::StringViewList &shv_path) override;
	shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override;

	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;

	shv::chainpack::RpcValue valueOnPath(const std::string &shv_path, bool throw_exc = true);
	virtual shv::chainpack::RpcValue valueOnPath(const StringViewList &shv_path, bool throw_exc = true);
	void setValueOnPath(const std::string &shv_path, const shv::chainpack::RpcValue &val);
	virtual void setValueOnPath(const StringViewList &shv_path, const shv::chainpack::RpcValue &val);

	void commitChanges() {saveValues();}

	void clearValuesCache() {m_valuesLoaded = false;}

	Q_SIGNAL void configSaved();
protected:
	static shv::chainpack::RpcValue valueOnPath(const shv::chainpack::RpcValue &val, const StringViewList &shv_path, bool throw_exc);
	virtual const shv::chainpack::RpcValue &values();
	virtual void loadValues();
	virtual void saveValues();
	bool isDir(const StringViewList &shv_path);
protected:
	bool m_valuesLoaded = false;
	shv::chainpack::RpcValue m_values;
};

class SHVIOTQT_DECL_EXPORT RpcValueConfigNode : public RpcValueMapNode
{
	Q_OBJECT

	using Super = RpcValueMapNode;

	SHV_FIELD_IMPL2(std::string, c, C, onfigName, "config")
	SHV_FIELD_IMPL2(std::string, c, C, onfigDir, ".")
	SHV_FIELD_IMPL(std::string, u, U, serConfigName)
	SHV_FIELD_IMPL(std::string, u, U, serConfigDir)
	SHV_FIELD_IMPL(std::string, t, T, emplateConfigName)
	SHV_FIELD_IMPL(std::string, t, T, emplateDir)
public:
	RpcValueConfigNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent);

	//shv::chainpack::RpcValue valueOnPath(const StringViewList &shv_path, bool throv_exc = true) override;
protected:
	//HNode* parentHNode();

	shv::chainpack::RpcValue loadConfigTemplate(const std::string &file_name);
	std::string resolvedUserConfigName();
	std::string resolvedUserConfigDir();

	void loadValues() override;
	void saveValues() override;

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod *metaMethod(const StringViewList &shv_path, size_t ix) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const chainpack::RpcValue &user_id) override;
protected:
	shv::chainpack::RpcValue m_templateValues;
};

/// Deprecated
class SHVIOTQT_DECL_EXPORT ObjectPropertyProxyShvNode : public shv::iotqt::node::ShvNode
{
	using Super = shv::iotqt::node::ShvNode;
public:
	//explicit MethodsTableNode(const std::string &node_id, shv::iotqt::node::ShvNode *parent = nullptr)
	//	: Super(node_id, parent) {}
	explicit ObjectPropertyProxyShvNode(const char *property_name, QObject *property_obj, shv::iotqt::node::ShvNode *parent = nullptr);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	//shv::chainpack::RpcValue callMethod(const shv::chainpack::RpcRequest &rq) override {return  Super::callMethod(rq);}
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
protected:
	QMetaProperty m_metaProperty;
	QObject *m_propertyObj = nullptr;
};

class SHVIOTQT_DECL_EXPORT ValueProxyShvNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT

	using Super = shv::iotqt::node::ShvNode;
public:
	enum class Type {
		Invalid = 0,
		Read = 1,
		Write = 2,
		ReadWrite = 3,
		Signal = 4,
		ReadSignal = 5,
		WriteSignal = 6,
		ReadWriteSignal = 7,
	};
	class SHVIOTQT_DECL_EXPORT Handle
	{
		friend class ValueProxyShvNode;
	public:
		Handle() {}
		virtual ~Handle();

		virtual shv::chainpack::RpcValue shvValue(int value_id) = 0;
		virtual void setShvValue(int value_id, const shv::chainpack::RpcValue &val) = 0;
		virtual std::string shvValueIdToName(int value_id) = 0;
		const shv::chainpack::RpcRequest& servedRpcRequest() const {return m_servedRpcRequest;}
	protected:
		shv::chainpack::RpcRequest m_servedRpcRequest;
	};
public:
	explicit ValueProxyShvNode(const std::string &node_id, int value_id, Type type, Handle *handled_obj, shv::iotqt::node::ShvNode *parent = nullptr);

	void addMetaMethod(shv::chainpack::MetaMethod &&mm);

	size_t methodCount(const StringViewList &shv_path) override;
	const shv::chainpack::MetaMethod* metaMethod(const StringViewList &shv_path, size_t ix) override;

	shv::chainpack::RpcValue callMethodRq(const shv::chainpack::RpcRequest &rq) override;
	shv::chainpack::RpcValue callMethod(const StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id) override;
protected:
	bool isWriteable() {return static_cast<int>(m_type) & static_cast<int>(Type::Write);}
	bool isReadable() {return static_cast<int>(m_type) & static_cast<int>(Type::Read);}
	bool isSignal() {return static_cast<int>(m_type) & static_cast<int>(Type::Signal);}

	Q_SLOT void onShvValueChanged(int value_id, shv::chainpack::RpcValue val);
protected:
	int m_valueId;
	Type m_type;
	Handle *m_handledObject = nullptr;
	std::vector<shv::chainpack::MetaMethod> m_extraMetaMethods;
};

}}}
