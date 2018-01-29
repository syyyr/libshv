#include "rpc.h"

#include "../core/log.h"

namespace shv {
namespace coreqt {
namespace chainpack {

void Rpc::registerMetatTypes()
{
	static auto rpcvalue_id = 0;
	if(rpcvalue_id == 0) {
		rpcvalue_id = qRegisterMetaType<shv::chainpack::RpcValue>();
		shvInfo() << "qRegisterMetaType<shv::chainpack::RpcValue>() =" << rpcvalue_id;
		//shvInfo() << NecroLog::instantiationInfo();
	}
}

} // namespace chainack
} // namespace coreqt
} // namespace shv
