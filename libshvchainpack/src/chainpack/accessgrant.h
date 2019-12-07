#pragma once

#include "../shvchainpackglobal.h"

#include "rpcvalue.h"

namespace shv {
namespace chainpack {

struct SHVCHAINPACK_DECL_EXPORT AccessGrant
{
	enum class LoginType {Invalid = 0, Plain, Sha1, RsaOaep};

	enum class Type { Invalid = 0, AccessLevel, Role, UserLogin, };
	Type type = Type::Invalid;

	bool notResolved = false;

	int accessLevel;

	std::string role;

	std::string user;
	std::string password;
	LoginType loginType;
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::AccessGrant};
		struct Key { enum Enum {Type = 1, NotResolved, Role, AccessLevel, User, Password, LoginType, MAX};};

		MetaType();
		static void registerMetaType();
	};
public:
	bool isValid() const;
	bool isUserLogin() const;
	bool isRole() const;
	bool isAccessLevel() const;

	chainpack::RpcValue toRpcValue() const;
	static AccessGrant fromRpcValue(const chainpack::RpcValue &rpcval);
};
#if 0
class SHVCHAINPACK_DECL_EXPORT AccessGrantRole : public AccessGrant
{
	using Super = AccessGrant;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::Role, r, setR, ole)
public:
	AccessGrantRole() : Super() {}
	AccessGrantRole(const std::string &role_name);
	AccessGrantRole(const std::string &role_name, bool not_resolved);
	AccessGrantRole(const AccessGrant &o) : Super(o) {}
};

class SHVCHAINPACK_DECL_EXPORT AccessGrantAccessLevel : public AccessGrant
{
	using Super = AccessGrant;

	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::AccessLevel, a, setA, ccessLevel)
public:
	AccessGrantAccessLevel() : Super() {}
	AccessGrantAccessLevel(int access_level);
	AccessGrantAccessLevel(const AccessGrant &o) : Super(o) {}
};

class SHVCHAINPACK_DECL_EXPORT AccessGrantUserLogin : public AccessGrant
{
	using Super = AccessGrant;

	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::User, u, setU, ser)
	SHV_IMAP_FIELD_IMPL(int, MetaType::Key::Password, p, setP, assword)
	SHV_IMAP_FIELD_IMPL(std::string, MetaType::Key::LoginType, l, setL, oginType)

public:
	AccessGrantUserLogin() : Super() {}
	AccessGrantUserLogin(const AccessGrant &o) : Super(o) {}
};
#endif
} // namespace chainpack
} // namespace shv
