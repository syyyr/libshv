#pragma once

#include "deviceconnection.h"

namespace shv {
namespace iotqt {
namespace rpc {
/*
class SHVIOTQT_DECL_EXPORT BrokerConnectionAppCliOptions : public DeviceAppCliOptions
{
	Q_OBJECT
private:
	using Super = DeviceAppCliOptions;
public:
	BrokerConnectionAppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "exports.shvPath", b, setB, rokerExportedShvPath)
};
*/
class SHVIOTQT_DECL_EXPORT SlaveBrokerConnection : public DeviceConnection
{
	Q_OBJECT
	using Super = DeviceConnection;
public:
	SlaveBrokerConnection(QObject *parent = nullptr);

	virtual void setOptions(const chainpack::RpcValue &slave_broker_options);
protected:
	//shv::chainpack::RpcValue createLoginParams(const shv::chainpack::RpcValue &server_hello) override;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
