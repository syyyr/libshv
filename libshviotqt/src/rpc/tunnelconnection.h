#pragma once

#include "clientappclioptions.h"
#include "clientconnection.h"
#include "tunnelhandle.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT TunnelAppCliOptions : public ClientAppCliOptions
{
	Q_OBJECT
private:
	using Super = ClientAppCliOptions;
public:
	TunnelAppCliOptions(QObject *parent = NULL);

	//CLIOPTION_GETTER_SETTER2(QString, "tunnel.parentClientId", p, setP, arentClientId)
	//CLIOPTION_GETTER_SETTER2(QString, "tunnel.tunnelName", t, setT, unnelName)
};

class SHVIOTQT_DECL_EXPORT TunnelParams : public shv::chainpack::RpcValue::IMap
{
	using Super = shv::chainpack::RpcValue::IMap;
public:
	class SHVIOTQT_DECL_EXPORT MetaType : public shv::chainpack::meta::MetaType
	{
		using Super = shv::chainpack::meta::MetaType;
	public:
		enum {ID = shv::chainpack::meta::GlobalNS::RegisteredMetaTypes::RpcTunnelParams};
		//struct Tag { enum Enum {RequestId = meta::Tag::USER,
		//						MAX};};
		struct Key { enum Enum {
				Host = 1,
				Port,
				User,
				Password,
				ParentClientId,
				CallerClientIds,
				//TunName,
				//TunnelResponseRequestId,
				MAX
			};
		};

		MetaType();

		static void registerMetaType();
	};
public:
	TunnelParams();
	TunnelParams(const shv::chainpack::RpcValue::IMap &m);
	//TunnelParams& operator=(TunnelParams &&o) {Super::operator =(std::move(o)); return *this;}

	shv::chainpack::RpcValue toRpcValue() const;

	shv::chainpack::RpcValue callerClientIds() const;
};

class SHVIOTQT_DECL_EXPORT TunnelConnection : public shv::iotqt::rpc::ClientConnection
{
	Q_OBJECT
	using Super = ClientConnection;

	SHV_FIELD_IMPL(shv::chainpack::RpcValue, t, T, unnelHandle)
public:
	TunnelConnection(QObject *parent = 0);

	//void setCliOptions(const TunnelAppCliOptions *cli_opts);
	const TunnelParams& tunnelParams() const {return m_tunnelParams;}
	void setTunnelParams(const shv::chainpack::RpcValue::IMap &m) {m_tunnelParams = m;}
private:
	TunnelParams m_tunnelParams;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
