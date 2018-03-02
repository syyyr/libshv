#pragma once

#include "../shviotqtglobal.h"

#include "clientconnection.h"

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT DeviceConnection : public ClientConnection
{
	Q_OBJECT
	using Super = ClientConnection;

	SHV_FIELD_IMPL(shv::chainpack::RpcValue, d, D, evice)

public:
	DeviceConnection(SyncCalls sync_calls, QObject *parent = 0);
protected:
	shv::chainpack::RpcValue createLoginParams(const shv::chainpack::RpcValue &server_hello) override;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
