#pragma once

#include "../shvchainpackglobal.h"

#include "rpcvalue.h"

namespace shv {
namespace chainpack {

struct SHVCHAINPACK_DECL_EXPORT UserLoginContext
{
	std::string serverNounce;
};

struct SHVCHAINPACK_DECL_EXPORT UserLogin
{
public:
	enum class LoginType {Invalid = 0, Plain, Sha1, RsaOaep};

	std::string user;
	std::string password;
	LoginType loginType;

	bool isValid() const {return !user.empty();}

	static const char *loginTypeToString(LoginType t);
	static LoginType loginTypeFromString(const std::string &s);
	static UserLogin fromRpcValue(const RpcValue &val);
};

struct SHVCHAINPACK_DECL_EXPORT AccessGrant
{

	enum class Type { Invalid = 0, AccessLevel, Role, UserLogin, };
	Type type = Type::Invalid;
	bool notResolved = false;
	int accessLevel;
	std::string role;
	UserLogin login;
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
	//static AccessGrant fromAccessLevel(int access_level);
};

struct SHVCHAINPACK_DECL_EXPORT PathAccessGrant : public AccessGrant
{
private:
	using Super = AccessGrant;
public:
	bool forwardGrantFromRequest = false;
};
} // namespace chainpack
} // namespace shv
