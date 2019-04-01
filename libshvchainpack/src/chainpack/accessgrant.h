#pragma once

#include "../shvchainpackglobal.h"

#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT AccessGrant : public shv::chainpack::RpcValue
{
	using Super = shv::chainpack::RpcValue;
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::RegisteredMetaTypes::AccessGrantLogin};
		/*
		struct Tag { enum Enum {RequestId = chainpack::meta::Tag::USER, // 8
								MAX};};
		*/
		struct Key { enum Enum {User = 1, Password, LoginType, MAX};};

		MetaType();

		static void registerMetaType();
	};
public:
	AccessGrant() : Super() {}
	AccessGrant(const shv::chainpack::RpcValue &o) : Super(o) {}

	bool isLogin() const { return type() == shv::chainpack::RpcValue::Type::IMap; }
	bool isGrantName() const { return type() == shv::chainpack::RpcValue::Type::String; }
	bool isAccessLevel() const { return type() == shv::chainpack::RpcValue::Type::Int; }
};

class SHVCHAINPACK_DECL_EXPORT AccessGrantLogin : public AccessGrant
{
	using Super = AccessGrant;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::User, u, setU, ser)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Password, p, setP, assword)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::LoginType, l, setL, oginType)

public:
	AccessGrantLogin() : Super() {}
	AccessGrantLogin(const AccessGrant &o) : Super(o) {}
};

class SHVCHAINPACK_DECL_EXPORT AccessGrantName : public AccessGrant
{
	using Super = AccessGrant;
public:
	AccessGrantName() : Super() {}
	AccessGrantName(const std::string &grant_name);
	AccessGrantName(const AccessGrant &o) : Super(o) {}
};

class SHVCHAINPACK_DECL_EXPORT AccessGrantLevel : public AccessGrant
{
	using Super = AccessGrant;
public:
	AccessGrantLevel() : Super() {}
	AccessGrantLevel(int access_level);
	AccessGrantLevel(const AccessGrant &o) : Super(o) {}
};

} // namespace chainpack
} // namespace shv
