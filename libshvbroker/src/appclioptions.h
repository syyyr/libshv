#pragma once

#include "shvbrokerglobal.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/clioptions.h>

#include <QSet>

namespace shv {
namespace broker {

class SHVBROKER_DECL_EXPORT AppCliOptions : public shv::core::utils::ConfigCLIOptions
{
private:
	using Super = shv::core::utils::ConfigCLIOptions;
public:
	AppCliOptions();
	//~AppCliOptions() Q_DECL_OVERRIDE {}

	CLIOPTION_GETTER_SETTER(std::string, l, setL, ocale)
	CLIOPTION_GETTER_SETTER2(int, "server.port", s, setS, erverPort)
#ifdef WITH_SHV_WEBSOCKETS
	CLIOPTION_GETTER_SETTER2(int, "server.websocket.port", s, setS, erverWebsocketPort)
#endif
	CLIOPTION_GETTER_SETTER2(std::string, "server.publicIP", p, setP, ublicIP)

	//CLIOPTION_GETTER_SETTER2(std::string, "etc.acl.fstab", f, setF, stabFile)
	//CLIOPTION_GETTER_SETTER2(std::string, "etc.acl.users", u, setU, sersFile)
	//CLIOPTION_GETTER_SETTER2(std::string, "etc.acl.grants", g, setG, rantsFile)
	//CLIOPTION_GETTER_SETTER2(std::string, "etc.acl.paths", p, setP, athsFile)

	CLIOPTION_GETTER_SETTER2(bool, "sqlconfig.enabled", is, set, SqlConfigEnabled)
	CLIOPTION_GETTER_SETTER2(std::string, "sqlconfig.driver", s, setS, qlConfigDriver)
	CLIOPTION_GETTER_SETTER2(std::string, "sqlconfig.database", s, setS, qlConfigDatabase)

	CLIOPTION_GETTER_SETTER2(shv::chainpack::RpcValue, "masters.connections", m, setM, asterBrokersConnections)
	CLIOPTION_GETTER_SETTER2(bool, "masters.enabled", is, set, MasterBrokersEnabled)
};

}}
