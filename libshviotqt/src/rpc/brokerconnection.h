#pragma once

#include "deviceconnection.h"

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT BrokerConnectionAppCliOptions : public DeviceAppCliOptions
{
	Q_OBJECT
private:
	using Super = DeviceAppCliOptions;
public:
	BrokerConnectionAppCliOptions(QObject *parent = nullptr);

	CLIOPTION_GETTER_SETTER2(QString, "exports.shvPath", e, setE, xportedShvPath)
};

class SHVIOTQT_DECL_EXPORT BrokerConnection : public DeviceConnection
{
	Q_OBJECT
	using Super = DeviceConnection;
public:
	BrokerConnection(QObject *parent = nullptr);

	void setCliOptions(const BrokerConnectionAppCliOptions *cli_opts);
protected:
	//shv::chainpack::RpcValue createLoginParams(const shv::chainpack::RpcValue &server_hello) override;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
