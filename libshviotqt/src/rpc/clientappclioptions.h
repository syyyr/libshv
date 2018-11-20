#pragma once

#include <shv/core/utils/clioptions.h>

#include "../shviotqtglobal.h"

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT ClientAppCliOptions : public shv::core::utils::ConfigCLIOptions
{
private:
	using Super = shv::core::utils::ConfigCLIOptions;
public:
	ClientAppCliOptions();
	//~ClientAppCliOptions() Q_DECL_OVERRIDE {}

	//CLIOPTION_GETTER_SETTER(QString, l, setL, ocale)
	CLIOPTION_GETTER_SETTER2(std::string, "login.user", u, setU, ser)
	CLIOPTION_GETTER_SETTER2(std::string, "login.password", p, setP, assword)
	CLIOPTION_GETTER_SETTER2(std::string, "login.passwordFile", p, setP, asswordFile)
	CLIOPTION_GETTER_SETTER2(std::string, "login.passwordFormat", p, setP, asswordFormat)
	CLIOPTION_GETTER_SETTER2(std::string, "login.type", l, setL, oginType)
	CLIOPTION_GETTER_SETTER2(std::string, "server.host", s, setS, erverHost)
	CLIOPTION_GETTER_SETTER2(int, "server.port", s, setS, erverPort)

	CLIOPTION_GETTER_SETTER2(std::string, "rpc.protocolType", p, setP, rotocolType)

	//CLIOPTION_GETTER_SETTER2(int, "rpc.timeout", r, setR, pcTimeout)
	CLIOPTION_GETTER_SETTER2(bool, "rpc.metaTypeExplicit", is, set, MetaTypeExplicit)
	CLIOPTION_GETTER_SETTER2(int, "rpc.reconnectInterval", r, setR, econnectInterval)
	CLIOPTION_GETTER_SETTER2(int, "rpc.heartbeatInterval", h, setH, eartbeatInterval)
};

} // namespace client
} // namespace iot
} // namespace shv
