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
		enum {ID = chainpack::meta::GlobalNS::RegisteredMetaTypes::TunnelCtl};
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
	/*
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::RequestId, r, setR, equestId)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)
	*/
public:
	//using Super::Super;
	TunnelCtl() {}
	TunnelCtl(State::Enum st);
	TunnelCtl(const RpcValue::Map &o);
	TunnelCtl(const RpcValue &o);

	//static TunnelCtl fromIMap(const shv::chainpack::RpcValue::IMap &m);
};

class SHVCHAINPACK_DECL_EXPORT FindTunnelRequest : public TunnelCtl
{
	using Super = TunnelCtl;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)

public:
	FindTunnelRequest() : Super(State::FindTunnelRequest) {}
	FindTunnelRequest(const TunnelCtl &o) : Super(o) {}
};

class SHVCHAINPACK_DECL_EXPORT FindTunnelResponse : public TunnelCtl
{
	using Super = TunnelCtl;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::RequestId, r, setR, equestId)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)

public:
	FindTunnelResponse() : Super(State::FindTunnelResponse) {}
	FindTunnelResponse(const TunnelCtl &o) : Super(o) {}

	static FindTunnelResponse fromFindTunnelRequest(const FindTunnelRequest &rq);

};

class SHVCHAINPACK_DECL_EXPORT CreateTunnelRequest : public TunnelCtl
{
	using Super = TunnelCtl;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Host, h, setH, ost)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Port, p, setP, ort)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Secret, s, setS, ecret)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::RequestId, r, setR, equestId)
	SHV_IMAP_FIELD_IMPL(shv::chainpack::RpcValue, MetaType::Key::CallerIds, c, setC, allerIds)

public:
	CreateTunnelRequest() : Super(State::CreateTunnelRequest) {}
	CreateTunnelRequest(const TunnelCtl &o) : Super(o) {}

	static CreateTunnelRequest fromFindTunnelResponse(const FindTunnelResponse &resp);
};

class SHVCHAINPACK_DECL_EXPORT CreateTunnelResponse : public TunnelCtl
{
	using Super = TunnelCtl;

public:
	CreateTunnelResponse() : Super(State::CreateTunnelResponse) {}
	CreateTunnelResponse(const TunnelCtl &o) : Super(o) {}
};

}}
