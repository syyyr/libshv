#include "deviceconnection.h"
#include "../utils/fileshvjournal.h"

#include <shv/core/log.h>
#include <shv/core/string.h>

#include <QCoreApplication>

#include <fstream>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

DeviceAppCliOptions::DeviceAppCliOptions()
{
	addOption("device.mountPoint").setType(cp::RpcValue::Type::String).setNames({"-m", "--mount", "--mount-point"}).setComment("Shv tree, where device should be mounted to. Only paths beginning with test/ are enabled. --mount-point version is deprecated");
	addOption("device.id").setType(cp::RpcValue::Type::String).setNames("--id", "--device-id").setComment("Device ID");
	addOption("device.idFile").setType(cp::RpcValue::Type::String).setNames("--idf", "--device-id-file").setComment("Device ID file");
	addOption("shvJournal.dir").setType(cp::RpcValue::Type::String).setNames("--jd", "--shvjournal-dir").setComment("SHV journal directory").setDefaultValue("/tmp/shvjournal/noname");
	addOption("shvJournal.fileSizeLimit").setType(cp::RpcValue::Type::String).setNames("--jfs", "--shvjournal-file-size-limit")
			.setComment("Maximum SHV journal file size, multiplicator postfix is possible, like 4K, 1M or 2G")
			.setDefaultValue(std::to_string(utils::FileShvJournal::DEFAULT_FILE_SIZE_LIMIT));
	addOption("shvJournal.sizeLimit").setType(cp::RpcValue::Type::String).setNames("--js", "--shvjournal-size-limit")
			.setComment("Maximum SHV journal size, multiplicator postfix is possible, like 4K, 1M or 2G")
			.setDefaultValue(std::to_string(utils::FileShvJournal::DEFAULT_JOURNAL_SIZE_LIMIT));
}

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
