#include "accessgrant.h"

namespace shv {
namespace chainpack {

//================================================================
// AccessGrant
//================================================================
AccessGrant::MetaType::MetaType()
	: Super("AccessGrantLogin")
{
	m_keys = {
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

//================================================================
// AccessGrantName
//================================================================
AccessGrantName::AccessGrantName(const std::string &grant_name)
	: Super(grant_name)
{
	MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
}

//================================================================
// AccessGrantName
//================================================================
AccessGrantLevel::AccessGrantLevel(int access_level)
	: Super(access_level)
{
	MetaType::registerMetaType();
	setMetaValue(chainpack::meta::Tag::MetaTypeId, MetaType::ID);
}

} // namespace chainpack
} // namespace shv
