#pragma once

#include <shv/iotqt/node/shvnode.h>

namespace shv {
namespace broker {

namespace rpc { class BrokerTcpServer; class ServerConnectionBroker; }

class ClientShvNode : public shv::iotqt::node::ShvNode
{
	Q_OBJECT
private:
	using Super = shv::iotqt::node::ShvNode;
public:
	ClientShvNode(rpc::ServerConnectionBroker *conn, shv::iotqt::node::ShvNode *parent = nullptr);
	~ClientShvNode() override;

	rpc::ServerConnectionBroker * connection() const {return m_connections.value(0);}
	QList<rpc::ServerConnectionBroker *> connections() const {return m_connections;}

	void addConnection(rpc::ServerConnectionBroker *conn);
	void removeConnection(rpc::ServerConnectionBroker *conn);

	void handleRawRpcRequest(shv::chainpack::RpcValue::MetaData &&meta, std::string &&data) override;
	shv::chainpack::RpcValue hasChildren(const StringViewList &shv_path) override;
private:
	QList<rpc::ServerConnectionBroker *> m_connections;
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
