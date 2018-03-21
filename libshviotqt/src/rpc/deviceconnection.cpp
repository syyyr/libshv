#include "deviceconnection.h"

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

DeviceConnection::DeviceConnection(QObject *parent)
	: Super(SyncCalls::Disabled, parent)
{
	setConnectionType(cp::Rpc::TYPE_DEVICE);
}
/*
chainpack::RpcValue DeviceConnection::createLoginParams(const chainpack::RpcValue &server_hello)
{
	chainpack::RpcValue v = Super::createLoginParams(server_hello);
	cp::RpcValue::Map m = v.toMap();
	m["device"] =  device();
	return m;
}
*/
} // namespace rpc
} // namespace iotqt
} // namespace shv
