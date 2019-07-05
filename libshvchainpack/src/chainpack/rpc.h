#pragma once

#include "../shvchainpackglobal.h"

#include <string>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT Rpc
{
public:
	enum class ProtocolType {Invalid = 0, ChainPack, Cpon, JsonRpc};
	static const char* protocolTypeToString(ProtocolType pv);

	struct SHVCHAINPACK_DECL_EXPORT AccessGrant
	{
		std::string grant;
		int weight = -1;

		AccessGrant() {}
		AccessGrant(const std::string &g, int w) : grant(g), weight(w) {}

		bool isValid() const {return weight >= 0;}
 	};

	static const char* OPT_IDLE_WD_TIMEOUT;

	static const char* KEY_OPTIONS;
	static const char* KEY_MOUT_POINT;
	static const char* KEY_SHV_PATH;
	static const char* KEY_DEVICE_ID;
	static const char* KEY_CLIENT_ID;
	static const char* KEY_LOGIN;
	static const char* KEY_DEVICE;
	static const char* KEY_TUNNEL;
	static const char* KEY_BROKER;
	static const char* KEY_SECRET;

	static const char* METH_HELLO;
	static const char* METH_LOGIN;
	static const char* METH_GET;
	static const char* METH_SET;
	static const char* METH_DIR;
	static const char* METH_LS;
	static const char* METH_PING;
	static const char* METH_ECHO;
	static const char* METH_APP_NAME;
	static const char* METH_DEVICE_ID;
	static const char* METH_DEVICE_TYPE;
	static const char* METH_CLIENT_ID;
	static const char* METH_GIT_COMMIT;
	static const char* METH_MOUNT_POINT;
	static const char* METH_SUBSCRIBE;
	static const char* METH_UNSUBSCRIBE;
	static const char* METH_REJECT_NOT_SUBSCRIBED;
	static const char* METH_RUN_CMD;
	static const char* METH_RUN_SCRIPT;
	static const char* METH_LAUNCH_REXEC;
	static const char* METH_HELP;
	static const char* METH_GET_LOG;

	static const char* PAR_PATH;
	static const char* PAR_METHOD;

	static const char* SIG_VAL_CHANGED;
	static const char* SIG_VAL_FASTCHANGED;
	static const char* SIG_SERVICE_VAL_CHANGED;
	static const char* SIG_MOUNTED_CHANGED;

	static const char* GRANT_BROWSE;
	static const char* GRANT_READ;
	static const char* GRANT_WRITE;
	static const char* GRANT_COMMAND;
	static const char* GRANT_CONFIG;
	static const char* GRANT_SERVICE;
	static const char* GRANT_DEVEL;
	static const char* GRANT_ADMIN;
	static const char* GRANT_MASTER_BROKER;

	static const char* DIR_BROKER;
	static const char* DIR_BROKER_APP;
	static const char* DIR_CLIENTS;
	static const char* DIR_MASTERS;

	static const char* JSONRPC_REQUEST_ID;
	static const char* JSONRPC_METHOD;
	static const char* JSONRPC_PARAMS;
	static const char* JSONRPC_RESULT;
	static const char* JSONRPC_ERROR;
	static const char* JSONRPC_ERROR_CODE;
	static const char* JSONRPC_ERROR_MESSAGE;
	static const char* JSONRPC_SHV_PATH;
	static const char* JSONRPC_CALLER_ID;
	static const char* JSONRPC_REV_CALLER_ID;
};

} // namespace chainpack
} // namespace shv
