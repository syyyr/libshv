#include "deviceconnection.h"
#include "../utils/fileshvjournal.h"

#include <shv/core/log.h>

#include <fstream>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

DeviceAppCliOptions::DeviceAppCliOptions(QObject *parent)
	: Super(parent)
{
	addOption("device.mountPoint").setType(QVariant::String).setNames({"-m", "--mount", "--mount-point"}).setComment(tr("Shv tree, where device should be mounted to. Only paths beginning with test/ are enabled. --mount-point version is deprecated"));
	addOption("device.id").setType(QVariant::String).setNames("--id", "--device-id").setComment(tr("Device ID"));
	addOption("device.idFile").setType(QVariant::String).setNames("--idf", "--device-id-file").setComment(tr("Device ID file"));
	addOption("shvJournal.dir").setType(QVariant::String).setNames("--jd", "--shvjournal-dir").setComment(tr("SHV journal directory"));
	addOption("shvJournal.fileSizeLimit").setType(QVariant::Int).setNames("--jfs", "--shvjournal-file-size-limit").setComment(tr("Maximum SHV journal file size")).setDefaultValue((int)utils::FileShvJournal::DEFAULT_FILE_SIZE_LIMIT);
	addOption("shvJournal.dirSizeLimit").setType(QVariant::Int).setNames("--jds", "--shvjournal-dir-size-limit").setComment(tr("Maximum SHV journal directory size")).setDefaultValue((int)utils::FileShvJournal::DEFAULT_JOURNAL_SIZE_LIMIT);
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
		std::string device_id = cli_opts->deviceId().toStdString();
		if(device_id.empty()) {
			std::string device_id_file = cli_opts->deviceIdFile().toStdString();
			if(!device_id.empty()) {
				std::ifstream ifs(device_id_file, std::ios::binary);
				if(ifs)
					ifs >> device_id;
				else
					shvError() << "Cannot read device ID file:" << device_id_file;
			}
		}
		if(!device_id.empty())
			dev[cp::Rpc::KEY_DEVICE_ID] = cli_opts->deviceId().toStdString();
		dev[cp::Rpc::KEY_MOUT_POINT] = cli_opts->mountPoint().toStdString();
		opts[cp::Rpc::TYPE_DEVICE] = dev;
		setConnectionOptions(opts);
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
