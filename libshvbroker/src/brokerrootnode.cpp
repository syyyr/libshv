#include "brokerrootnode.h"

#include <shv/coreqt/log.h>

namespace shv {
namespace broker {

BrokerRootNode::BrokerRootNode(shv::iotqt::node::ShvNode *parent)
	: Super(parent)
{
}
/*
chainpack::RpcValue BrokerRootNode::callMethodRq(const chainpack::RpcRequest &rq)
{
	shvDebug() << "request:" << rq.toCpon();
	chainpack::RpcValue res = Super::callMethodRq(rq);
	shvDebug() << "result:" << res.toCpon();
	if (rq.metaData().hasKey(rpc::MasterBrokerConnection::ADD_LOCAL_TO_LS_RESULT_META_KEY)) {
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
*/
}
}

