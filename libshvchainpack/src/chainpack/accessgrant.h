#pragma once

#include "../shvchainpackglobal.h"

#include "rpcmessage.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

struct UserLogin;

struct SHVCHAINPACK_DECL_EXPORT UserLoginContext
{
	std::string serverNounce;
	std::string clientType;
	shv::chainpack::RpcRequest loginRequest;
	int connectionId = 0;

	const RpcValue::Map &loginParams() const;
	shv::chainpack::UserLogin userLogin() const;
};

struct SHVCHAINPACK_DECL_EXPORT UserLoginResult
{
	bool passwordOk = false;
	std::string loginError;
	int clientId = 0;

	UserLoginResult() {}
	UserLoginResult(bool password_ok) : UserLoginResult(password_ok, std::string()) {}
	UserLoginResult(bool password_ok, std::string login_error)
		: passwordOk(password_ok)
		, loginError(std::move(login_error)) {}

	const RpcValue::Map &loginParams() const;
	shv::chainpack::UserLogin userLogin() const;
	shv::chainpack::RpcValue toRpcValue() const;
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
	chainpack::RpcValue toRpcValueMap() const;
	static UserLogin fromRpcValue(const RpcValue &val);
};

struct SHVCHAINPACK_DECL_EXPORT AccessGrant
{

	enum class Type { Invalid = 0, AccessLevel, Role, UserLogin, };
	Type type = Type::Invalid;
	// acces grant sent by client or forwarded by master broker is not resolved through 'paths' table
	// resolved grant is not translated in slave broker's 'paths' table when rpc message is sent to client
	bool isResolved = false;
	int accessLevel;
	std::string role;
	UserLogin login;
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::AccessGrant};
		struct Key { enum Enum {Type = 1, IsResolved, Role, AccessLevel, User, Password, LoginType, MAX};};

		MetaType();
		static void registerMetaType();
	};
public:
	bool isValid() const;
	bool isUserLogin() const;
	bool isRole() const;
	bool isAccessLevel() const;

	chainpack::RpcValue toRpcValue() const;
	chainpack::RpcValue toRpcValueMap() const;
	static AccessGrant fromRpcValue(const chainpack::RpcValue &rpcval);
	static const char* typeToString(Type t);
};

struct SHVCHAINPACK_DECL_EXPORT PathAccessGrant : public AccessGrant
{
private:
	using Super = AccessGrant;
public:
	bool forwardGrantFromRequest = false;

	PathAccessGrant() {}
	PathAccessGrant(Super &&o) : Super(std::move(o)) {}

	chainpack::RpcValue toRpcValueMap() const;
	static PathAccessGrant fromRpcValue(const chainpack::RpcValue &rpcval);
};
} // namespace chainpack
} // namespace shv
