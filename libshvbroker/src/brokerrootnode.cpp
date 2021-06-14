#include "brokerrootnode.h"

namespace shv {
namespace broker {

BrokerRootNode::BrokerRootNode(shv::iotqt::node::ShvNode *parent)
	: Super(parent)
{
}

chainpack::RpcValue BrokerRootNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	chainpack::RpcValue res = Super::processRpcRequest(rq);
	if (rq.metaData().hasKey("addLocal")) {
		chainpack::RpcValue::List res_list = res.asList();
		if (res_list.size() && !res_list[0].isList()) {
			res_list.push_back(".local");
		}
		else {
			res_list.push_back(chainpack::RpcValue::List{ ".local", true });
		}
		res = res_list;
	}
	return res;
}


}
}

