#include "brokerrootnode.h"
#include "rpc/masterbrokerconnection.h"

namespace shv {
namespace broker {

BrokerRootNode::BrokerRootNode(shv::iotqt::node::ShvNode *parent)
	: Super(parent)
{
}

chainpack::RpcValue BrokerRootNode::callMethodRq(const chainpack::RpcRequest &rq)
{
	chainpack::RpcValue res = Super::callMethodRq(rq);
	if (rq.metaData().hasKey(rpc::MasterBrokerConnection::LOCAL_INTERNAL_META_KEY)) {
		chainpack::RpcValue::List res_list = res.asList();
		if (res_list.size() && !res_list[0].isList()) {
			res_list.push_back(rpc::MasterBrokerConnection::LOCAL_NODE);
		}
		else {
			res_list.push_back(chainpack::RpcValue::List{ rpc::MasterBrokerConnection::LOCAL_NODE, true });
		}
		res = res_list;
	}
	return res;
}


}
}

