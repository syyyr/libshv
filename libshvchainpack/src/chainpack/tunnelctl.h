#pragma once

#include "../shvchainpackglobal.h"

#include "rpcvalue.h"
#include "utils.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT TunnelCtl : public shv::chainpack::RpcValue
{
	using Super = shv::chainpack::RpcValue;
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::TunnelCtl};
		/*
		struct Tag { enum Enum {RequestId = chainpack::meta::Tag::USER, // 8
								MAX};};
		*/
		struct Key { enum Enum {State = 1, Host, Port, Secret, RequestId, CallerIds, MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	struct State {enum Enum {
			Invalid = 0,
			FindTunnelRequest,
			FindTunnelResponse,
			CreateTunnelRequest,
			CreateTunnelResponse,
			CloseTunnel,
		};};

	SHV_IMAP_FIELD_IMPL2(int, MetaType::Key::State, s, setS, tate, State::Invalid)
public:
	//using Super::Super;
	TunnelCtl() = default;
	TunnelCtl(State::Enum st);
	//TunnelCtl(const RpcValue::IMap &o);
	TunnelCtl(const RpcValue &o);

	//static TunnelCtl fromIMap(const shv::chainpack::RpcValue::IMap &m);
};

class SHVCHAINPACK_DECL_EXPORT FindTunnelReqCtl : public TunnelCtl
{
	using Super = TunnelCtl;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)

public:
	FindTunnelReqCtl() : Super(State::FindTunnelRequest) {}
	FindTunnelReqCtl(const TunnelCtl &o) : Super(o) {}
};

class SHVCHAINPACK_DECL_EXPORT FindTunnelRespCtl : public TunnelCtl
{
	using Super = TunnelCtl;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::RequestId, r, setR, equestId)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)

public:
	FindTunnelRespCtl() : Super(State::FindTunnelResponse) {}
	FindTunnelRespCtl(const TunnelCtl &o) : Super(o) {}

	static FindTunnelRespCtl fromFindTunnelRequest(const FindTunnelReqCtl &rq);

};

class SHVCHAINPACK_DECL_EXPORT CreateTunnelReqCtl : public TunnelCtl
{
	using Super = TunnelCtl;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::RequestId, r, setR, equestId)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)

public:
	CreateTunnelReqCtl() : Super(State::CreateTunnelRequest) {}
	CreateTunnelReqCtl(const TunnelCtl &o) : Super(o) {}

	static CreateTunnelReqCtl fromFindTunnelResponse(const FindTunnelRespCtl &resp);
};

class SHVCHAINPACK_DECL_EXPORT CreateTunnelRespCtl : public TunnelCtl
{
	using Super = TunnelCtl;

public:
	CreateTunnelRespCtl() : Super(State::CreateTunnelResponse) {}
	CreateTunnelRespCtl(const TunnelCtl &o) : Super(o) {}
};

class SHVCHAINPACK_DECL_EXPORT CloseTunnelCtl : public TunnelCtl
{
	using Super = TunnelCtl;

public:
	CloseTunnelCtl() : Super(State::CreateTunnelResponse) {}
	CloseTunnelCtl(const TunnelCtl &o) : Super(o) {}
};

}}
