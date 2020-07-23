#include "appclioptions.h"

namespace cp = shv::chainpack;

namespace shv {
namespace broker {

AppCliOptions::AppCliOptions()
{
	addOption("locale").setType(cp::RpcValue::Type::String).setNames("--locale").setComment("Application locale").setDefaultValue("system");
	addOption("server.port").setType(cp::RpcValue::Type::Int).setNames("-p", "--server-port").setComment("Server port").setDefaultValue(3755);
	//addOption("server.websocket.enabled").setType(cp::RpcValue::Type::Bool).setNames("--ws", "--server-ws-enabled").setComment("Server web socket enabled").setDefaultValue(3777);
#ifdef WITH_SHV_WEBSOCKETS
	addOption("server.websocket.port").setType(cp::RpcValue::Type::Int).setNames("--server-ws-port")
			.setComment("Web socket server port, set this option to enable websocket server").setDefaultValue(3777);
	addOption("server.websocket.sslport").setType(cp::RpcValue::Type::Int).setNames("--server-wss-port")
			.setComment("Secure web socket server port, set this option to enable secure websocket server").setDefaultValue(3778);
	addOption("server.ssl.key").setType(cp::RpcValue::Type::String).setNames("--server-ssl-key")
			.setComment("SSL key file").setDefaultValue("wss.key");
	addOption("server.ssl.cert").setType(cp::RpcValue::Type::String).setNames("--server-ssl-cert")
			.setComment("SSL certificate file").setDefaultValue("wss.crt");
#endif
	addOption("server.publicIP").setType(cp::RpcValue::Type::String).setNames("--pip", "--server-public-ip").setComment("Server public IP address");
	addOption("sqlconfig.enabled").setType(cp::RpcValue::Type::Bool).setNames("--sql-config-enabled")
			.setComment("SQL config enabled")
			.setDefaultValue(false);
	addOption("sqlconfig.driver").setType(cp::RpcValue::Type::String).setNames("--sql-config-driver")
			.setComment("SQL config database driver.")
			.setDefaultValue("QSQLITE");
	addOption("sqlconfig.database").setType(cp::RpcValue::Type::String).setNames("--sql-config-db")
			.setComment("SQL config database, if it is relative path for SQLite, {config-dir} is prepended.")
			.setDefaultValue("shvbroker.cfg.db");

	addOption("masters.connections").setType(cp::RpcValue::Type::Map).setComment("Can be used from config file only.");
	addOption("masters.enabled").setType(cp::RpcValue::Type::Bool).setNames("--mce", "--master-connections-enabled").setComment("Enable slave connections to master broker.");

	addOption("master.broker.device.id").setType(shv::chainpack::RpcValue::Type::String).setNames("--master-broker-device-id").setComment("Master broker device ID");
}

}}
