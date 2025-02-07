#include "metatypes.h"
#include "exception.h"

#include <necrolog.h>

#include <map>

namespace shv {
namespace chainpack {
namespace meta {

const MetaInfo &MetaType::tagById(int id) const
{
	static std::map<int, MetaInfo> embeded_mi = {
		{(int)Tag::Invalid, {(int)Tag::Invalid, ""}},
		{(int)Tag::MetaTypeId, {(int)Tag::MetaTypeId, "T"}},
		{(int)Tag::MetaTypeNameSpaceId, {(int)Tag::MetaTypeNameSpaceId, "NS"}},
	};
	if(id <= (int)Tag::MetaTypeNameSpaceId)
		return embeded_mi[id];
	auto it = m_tags.find(id);
	if(it == m_tags.end())
		return embeded_mi[(int)Tag::Invalid];
	return m_tags.at(id);
}

const MetaInfo &MetaType::keyById(int id) const
{
	static MetaInfo invalid(-1, "");
	auto it = m_keys.find(id);
	if(it == m_keys.end())
		return invalid;
	return m_keys.at(id);
}

GlobalNS::GlobalNS()
	: Super("global")
{
}

#define DEFINE_GLOBALNS_METATYPE(name) \
	class name##_globalns_MetaType : public MetaType \
	{ \
		using Super = MetaType; \
	public: \
		enum {ID = GlobalNS::MetaTypeId::name}; \
		name##_globalns_MetaType() : Super(#name) {} \
	};


DEFINE_GLOBALNS_METATYPE(RpcConnectionParams)
DEFINE_GLOBALNS_METATYPE(TunnelCtl)
DEFINE_GLOBALNS_METATYPE(AccessGrant)
DEFINE_GLOBALNS_METATYPE(DataChange)
DEFINE_GLOBALNS_METATYPE(NodeDrop)
DEFINE_GLOBALNS_METATYPE(ValueNotAvailable)
DEFINE_GLOBALNS_METATYPE(NodePropertyMap)

#define STATIC_GLOBALNS_INSTANCE(name) \
{ \
	static name##_globalns_MetaType s; \
	registerType(GlobalNS::ID, name##_globalns_MetaType::ID, &s); \
}

void GlobalNS::registerMetaTypes()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		if(!registeredNameSpace(GlobalNS::ID).isValid()) {
			static GlobalNS ns;
			registerNameSpace(GlobalNS::ID, &ns);
		}
		STATIC_GLOBALNS_INSTANCE(RpcConnectionParams)
		STATIC_GLOBALNS_INSTANCE(TunnelCtl)
		STATIC_GLOBALNS_INSTANCE(AccessGrant)
		STATIC_GLOBALNS_INSTANCE(DataChange)
		STATIC_GLOBALNS_INSTANCE(NodeDrop)
		STATIC_GLOBALNS_INSTANCE(ValueNotAvailable)
		STATIC_GLOBALNS_INSTANCE(NodePropertyMap)
	}
}

namespace {

std::map<int, MetaNameSpace*>& registered_ns() {
	static std::map<int, MetaNameSpace*> s;
	return s;
}

}

static void initMetaTypes()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		{
			static GlobalNS def;
			registerNameSpace(GlobalNS::ID, &def);
		}
		{
			static ElesysNS def;
			registerNameSpace(ElesysNS::ID, &def);
		}
	}
}

void registerNameSpace(int ns_id, MetaNameSpace *ns)
{
	initMetaTypes();
	if(ns == nullptr)
		registered_ns().erase(ns_id);
	else
		registered_ns()[ns_id] = ns;
}

void registerType(int ns_id, int type_id, MetaType *type)
{
	initMetaTypes();
	auto it = registered_ns().find(ns_id);
	if(it == registered_ns().end())
		SHVCHP_EXCEPTION("Unknown namespace id!");
	if(type == nullptr)
		it->second->types().erase(type_id);
	else
		it->second->types()[type_id] = type;
}

const MetaNameSpace &registeredNameSpace(int ns_id)
{
	initMetaTypes();
	static MetaNameSpace invalid("");
	auto it = registered_ns().find(ns_id);
	if(it == registered_ns().end())
		return invalid;
	return *(it->second);
}

const MetaType &registeredType(int ns_id, int type_id)
{
	static MetaType invalid("");
	const MetaNameSpace &ns = registeredNameSpace(ns_id);
	if(ns.isValid()) {
		auto it = ns.types().find(type_id);
		if(it != ns.types().end()) {
			return *(it->second);
		}
	}
	return invalid;
}

ElesysNS::ElesysNS()
	: Super("elesys")
{
}

}}}
