#include "accessgrant.h"

#include <necrolog.h>

namespace shv {
namespace chainpack {

//================================================================
// AccessGrant
//================================================================
AccessGrant::MetaType::MetaType()
	: Super("AccessGrant")
{
	m_keys = {
		RPC_META_KEY_DEF(Type),
		RPC_META_KEY_DEF(NotResolved),
		RPC_META_KEY_DEF(Role),
		RPC_META_KEY_DEF(AccessLevel),
		RPC_META_KEY_DEF(User),
		RPC_META_KEY_DEF(Password),
		RPC_META_KEY_DEF(LoginType),
	};
	//m_tags = {
	//	RPC_META_TAG_DEF(shvPath)
	//};
}

void AccessGrant::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}
/*
AccessGrant::AccessGrant()
	: Super()
{
}

AccessGrant::AccessGrant(const RpcValue &o)
	: Super(o)
{
	MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
}
*/
bool AccessGrant::isValid() const
{
	return type != Type::Invalid;
}

bool AccessGrant::isUserLogin() const
{
	return type == Type::UserLogin;
}

bool AccessGrant::isRole() const
{
	return type == Type::Role;
}

bool AccessGrant::isAccessLevel() const
{
	return type == Type::AccessLevel;
}

RpcValue AccessGrant::toRpcValue() const
{
	if(!isValid())
		return RpcValue();
	if(!notResolved) {
		if(isAccessLevel())
			return RpcValue(accessLevel);
		if(isRole())
			return RpcValue(role);
	}
	RpcValue ret(RpcValue::IMap{});
	MetaType::registerMetaType();
	ret.setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
	ret.set(MetaType::Key::Type, (int)type);
	switch (type) {
	case Type::AccessLevel:
		ret.set(MetaType::Key::AccessLevel, accessLevel);
		break;
	case Type::Role:
		ret.set(MetaType::Key::Role, role);
		break;
	case Type::UserLogin:
		ret.set(MetaType::Key::User, user);
		ret.set(MetaType::Key::Password, password);
		ret.set(MetaType::Key::LoginType, (int)loginType);
		break;
	case Type::Invalid:
		break;
	}
	return ret;
}

AccessGrant AccessGrant::fromRpcValue(const RpcValue &rpcval)
{
	AccessGrant ret;
	switch (rpcval.type()) {
	case RpcValue::Type::UInt:
	case RpcValue::Type::Int:
		ret.type = Type::AccessLevel;
		ret.accessLevel = rpcval.toInt();
		break;
	case RpcValue::Type::String:
		ret.type = Type::Role;
		ret.role = rpcval.toString();
		break;
	case RpcValue::Type::IMap: {
		const RpcValue::IMap &m = rpcval.toIMap();
		ret.type = static_cast<Type>(m.value(MetaType::Key::Type).toInt());
		switch (ret.type) {
		case Type::AccessLevel:
			ret.accessLevel = m.value(MetaType::Key::AccessLevel).toInt();
			break;
		case Type::Role:
			ret.role = m.value(MetaType::Key::Role).toString();
			break;
		case Type::UserLogin:
			ret.user = m.value(MetaType::Key::User).toString();
			ret.password = m.value(MetaType::Key::Password).toString();
			ret.loginType = static_cast<LoginType>(m.value(MetaType::Key::LoginType).toInt());
			break;
		default:
			nError() << "Invalid access grant type:" << rpcval.toCpon();
			break;
		}
		break;
	}
	default:
		nError() << "Invalid access grant:" << rpcval.toCpon();
		break;
	}
	return ret;
}
#if 0
//================================================================
// AccessGrantRole
//================================================================
AccessGrantRole::AccessGrantRole(const std::string &role_name)
	: Super(role_name)
{
}

AccessGrantRole::AccessGrantRole(const std::string &role_name, bool not_resolved)
	: Super(RpcValue::Map{})
{
	setGrantType(GrantType::Role);
	setRole(role_name);
	setNotResolved(not_resolved);
}

//================================================================
// AccessGrantName
//================================================================
AccessGrantAccessLevel::AccessGrantAccessLevel(int access_level)
	: Super(access_level)
{
}
#endif

} // namespace chainpack
} // namespace shv
