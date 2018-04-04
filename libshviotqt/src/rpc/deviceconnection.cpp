#include "deviceconnection.h"

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

DeviceAppCliOptions::DeviceAppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("device.mountPoint").setType(QVariant::String).setNames({"-m", "--mount", "--mount-point"}).setComment(tr("Shv tree, where device should be mounted to. Only paths beginning with test/ are enabled. --mount-point version is deprecated"));
	addOption("device.id").setType(QVariant::String).setNames("-id", "--device-id").setComment(tr("Device ID"));
}

DeviceConnection::DeviceConnection(QObject *parent)
	: Super(SyncCalls::Disabled, parent)
{
	setConnectionType(cp::Rpc::TYPE_DEVICE);
}

void DeviceConnection::setCliOptions(const DeviceAppCliOptions *cli_opts)
{
	Super::setCliOptions(cli_opts);
	if(cli_opts) {
		chainpack::RpcValue::Map opts = connectionOptions().toMap();
		cp::RpcValue::Map dev;
		dev[cp::Rpc::KEY_DEVICE_ID] = cli_opts->deviceId().toStdString();
		dev[cp::Rpc::KEY_MOUT_POINT] = cli_opts->mountPoint().toStdString();
		opts[cp::Rpc::TYPE_DEVICE] = dev;
		setconnectionOptions(opts);
	}
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
