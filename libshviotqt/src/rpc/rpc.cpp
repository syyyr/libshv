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

shv::chainpack::RpcValue::DateTime Rpc::toRpcDateTime(const QDateTime &d)
{
	if (d.isValid()) {
		shv::chainpack::RpcValue::DateTime dt = shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(d.toUTC().toMSecsSinceEpoch());
		int offset = d.offsetFromUtc();
		dt.setTimeZone(offset / 60);
		return dt;
	}
	else {
		return shv::chainpack::RpcValue::DateTime();
	}
}

} // namespace chainack
} // namespace iotqt
} // namespace shv


