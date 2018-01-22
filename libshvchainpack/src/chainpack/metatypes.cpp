#include "metatypes.h"

#include "../necrolog/necrolog.h"
#include "exception.h"

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

ChainPackNS::ChainPackNS()
	: Super("ChainPack")
{
}

namespace {

std::map<int, MetaNameSpace*> registered_ns;

}

static void initMetaTypes()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static ChainPackNS def;
		registerNameSpace(ChainPackNS::ID, &def);
	}
}

void registerNameSpace(int ns_id, MetaNameSpace *ns)
{
	initMetaTypes();
	if(ns == nullptr)
		registered_ns.erase(ns_id);
	else
		registered_ns[ns_id] = ns;
}

void registerType(int ns_id, int type_id, MetaType *type)
{
	initMetaTypes();
	auto it = registered_ns.find(ns_id);
	if(it == registered_ns.end())
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
	auto it = registered_ns.find(ns_id);
	if(it == registered_ns.end())
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

}}}
