#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace shv {
namespace broker {

namespace rpc { class BrokerTcpServer; class ClientConnection; }

class ClientShvNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	ClientShvNode(const std::string &node_id, rpc::ClientConnection *conn, shv::iotqt::node::ShvNode *parent = nullptr);
	~ClientShvNode() override;

	rpc::ClientConnection * connection() const {return m_connections.value(0);}
	QList<rpc::ClientConnection *> connections() const {return m_connections;}

	void addConnection(rpc::ClientConnection *conn);
	void removeConnection(rpc::ClientConnection *conn);

	void handleRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, std::string &&data) override;
	shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override;
private:
	QList<rpc::ClientConnection *> m_connections;
};

class MasterBrokerShvNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	MasterBrokerShvNode(shv::iotqt::node::ShvNode *parent = nullptr);
	~MasterBrokerShvNode() override;
};

}}
