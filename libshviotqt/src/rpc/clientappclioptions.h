#pragma once

#include <shv/coreqt/utils/clioptions.h>

#include "../shviotqtglobal.h"

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT ClientAppCliOptions : public shv::coreqt::utils::ConfigCLIOptions
{
	Q_OBJECT
private:
	using Super = shv::coreqt::utils::ConfigCLIOptions;
public:
	ClientAppCliOptions(QObject *parent = NULL);
	~ClientAppCliOptions() Q_DECL_OVERRIDE {}

	//CLIOPTION_GETTER_SETTER(QString, l, setL, ocale)
	CLIOPTION_GETTER_SETTER2(QString, "login.user", u, setU, ser)
	CLIOPTION_GETTER_SETTER2(QString, "login.password", p, setP, assword)
	CLIOPTION_GETTER_SETTER2(QString, "server.host", s, setS, erverHost)
	CLIOPTION_GETTER_SETTER2(int, "server.port", s, setS, erverPort)

	CLIOPTION_GETTER_SETTER2(QString, "rpc.protocolType", p, setP, rotocolType)

	CLIOPTION_GETTER_SETTER2(int, "rpc.timeout", r, setR, pcTimeout)
	CLIOPTION_GETTER_SETTER2(bool, "rpc.metaTypeExplicit", is, set, MetaTypeExplicit)
	CLIOPTION_GETTER_SETTER2(int, "rpc.heartbeatInterval", h, setH, eartbeatInterval)

	CLIOPTION_GETTER_SETTER2(QString, "shv.mount", m, setM, ountPoint)
	CLIOPTION_GETTER_SETTER2(QString, "shv.deviceId", d, setD, eviceId)
};

} // namespace client
} // namespace iot
} // namespace shv
