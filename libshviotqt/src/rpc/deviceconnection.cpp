#include "deviceconnection.h"
#include "deviceappclioptions.h"

#include <shv/core/log.h>
#include <shv/core/string.h>

#include <QCoreApplication>

#include <fstream>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

DeviceConnection::DeviceConnection(QObject *parent)
	: Super(parent)
{
	//setConnectionType(cp::Rpc::TYPE_DEVICE);
}

const shv::chainpack::RpcValue::Map& DeviceConnection::deviceOptions() const
{
	return connectionOptions().toMap().value(cp::Rpc::KEY_DEVICE).toMap();
}

shv::chainpack::RpcValue DeviceConnection::deviceId() const
{
	return deviceOptions().value(cp::Rpc::KEY_DEVICE_ID);
}

void DeviceConnection::setCliOptions(const DeviceAppCliOptions *cli_opts)
{
	Super::setCliOptions(cli_opts);
	if(cli_opts) {
		chainpack::RpcValue::Map opts = connectionOptions().toMap();
		cp::RpcValue::Map dev;
		std::string device_id = cli_opts->deviceId();
		std::string device_id_file;
		{
			std::string fn = cli_opts->deviceIdFile();
			if(!fn.empty()) {
				std::ifstream ifs(fn, std::ios::binary);
				if(ifs)
					ifs >> device_id_file;
				else
					shvError() << "Cannot read device ID file:" << fn;
			}
		}
		if(device_id.empty()) {
			device_id = device_id_file;
		}
		else {
			shv::core::String::replace(device_id, "{{deviceIdFile}}", device_id_file);
			shv::core::String::replace(device_id, "{{appName}}", QCoreApplication::applicationName().toStdString());
		}
		if(!device_id.empty())
			dev[cp::Rpc::KEY_DEVICE_ID] = device_id;
		dev[cp::Rpc::KEY_MOUT_POINT] = cli_opts->mountPoint();
		opts[cp::Rpc::KEY_DEVICE] = dev;
		setConnectionOptions(opts);
	}
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
