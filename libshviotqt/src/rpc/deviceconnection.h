#pragma once

#include "clientappclioptions.h"
#include "clientconnection.h"

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT DeviceAppCliOptions : public ClientAppCliOptions
{
private:
	using Super = ClientAppCliOptions;
public:
	DeviceAppCliOptions();

	CLIOPTION_GETTER_SETTER2(std::string, "device.mountPoint", m, setM, ountPoint)
	CLIOPTION_GETTER_SETTER2(std::string, "device.id", d, setD, eviceId)
	CLIOPTION_GETTER_SETTER2(std::string, "device.idFile", d, setD, eviceIdFile)
	CLIOPTION_GETTER_SETTER2(std::string, "shvJournal.dir", s, setS, hvJournalDir)
	CLIOPTION_GETTER_SETTER2(int, "shvJournal.fileSizeLimit", s, setS, hvJournalFileSizeLimit)
	CLIOPTION_GETTER_SETTER2(int, "shvJournal.journalSizeLimit", s, setS, hvJournalSizeLimit)
};

class SHVIOTQT_DECL_EXPORT DeviceConnection : public ClientConnection
{
	Q_OBJECT
	using Super = ClientConnection;
public:
	DeviceConnection(QObject *parent = nullptr);

	const shv::chainpack::RpcValue::Map& deviceOptions() const;
	shv::chainpack::RpcValue deviceId() const;

	void setCliOptions(const DeviceAppCliOptions *cli_opts);
protected:
	//shv::chainpack::RpcValue createLoginParams(const shv::chainpack::RpcValue &server_hello) override;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
