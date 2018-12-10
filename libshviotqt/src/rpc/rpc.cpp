#include "rpc.h"

#include <shv/coreqt/log.h>

namespace shv {
namespace iotqt {
namespace rpc {

void Rpc::registerMetaTypes()
{
	static int rpcvalue_id = 0;
	if(rpcvalue_id == 0) {
		rpcvalue_id = qRegisterMetaType<shv::chainpack::RpcValue>();
		shvInfo() << "qRegisterMetaType<shv::chainpack::RpcValue>() =" << rpcvalue_id;
		//rpcvalue_id = qRegisterMetaType<std::string>();
		//shvInfo() << "qRegisterMetaType<std::string>() =" << rpcvalue_id;
	}
}

} // namespace chainack
} // namespace iotqt
} // namespace shv
