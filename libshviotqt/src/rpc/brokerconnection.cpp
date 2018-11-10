#include "brokerconnection.h"
#include "../utils/fileshvjournal.h"

#include <shv/core/log.h>

#include <fstream>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace rpc {

BrokerConnectionAppCliOptions::BrokerConnectionAppCliOptions(QObject *parent)
	: Super(parent)
{
	removeOption("shvJournal.dir");
	removeOption("shvJournal.fileSizeLimit");
	removeOption("shvJournal.dirSizeLimit");

	addOption("exports.shvPath").setType(QVariant::String).setNames("--export-path").setComment(tr("Exported SHV path"));
}

BrokerConnection::BrokerConnection(QObject *parent)
	: Super(parent)
{
}

void BrokerConnection::setCliOptions(const BrokerConnectionAppCliOptions *cli_opts)
{
	Super::setCliOptions(cli_opts);
	if(cli_opts) {
		chainpack::RpcValue::Map opts = connectionOptions().toMap();
		cp::RpcValue::Map broker;
		cp::RpcValue::Map exports;
		std::string export_shv_path = cli_opts->exportedShvPath().toStdString();
		exports["shvPath"] = export_shv_path;
		broker["exports"] = exports;
		opts["broker"] = broker;
		setConnectionOptions(opts);
	}
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
