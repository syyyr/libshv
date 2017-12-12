#include "metatypes.h"

#include "../core/log.h"
#include "../core/shvexception.h"

#include <map>

namespace shv {
namespace core {
namespace chainpack {

const MetaTypes::MetaInfo &MetaTypes::Type::tagById(int id) const
{
	static std::map<int, MetaInfo> embeded_mi = {
		{(int)Tag::Invalid, {(int)Tag::Invalid, ""}},
		{(int)Tag::MetaTypeId, {(int)Tag::MetaTypeId, "MetaTypeId"}},
		{(int)Tag::MetaTypeId, {(int)Tag::MetaTypeId, "MetaTypeNameSpaceId"}},
	};
	if(id <= (int)Tag::MetaTypeId)
		return embeded_mi[id];
	auto it = m_tags.find(id);
	if(it == m_tags.end())
		return embeded_mi[(int)Tag::Invalid];
	return m_tags.at(id);
}

const MetaTypes::MetaInfo &MetaTypes::Type::keyById(int id) const
{
	static MetaInfo invalid(-1, "");
	auto it = m_keys.find(id);
	if(it == m_keys.end())
		return invalid;
	return m_keys.at(id);
}

ns::Default::Default()
	: Super("Default")
{
}

namespace {

std::map<int, MetaTypes::NameSpace*> registered_ns;

}
#if 0
namespace
{

struct RegisteredNamespace
{
	MetaNameSpace *metaNameSpace = nullptr;
	std::map<int, MetaType*> registeredTypes;
};
std::map<int, RegisteredNamespace> registered_ns;

NS_Default ns_Default\;
}

NS_Default::NS_Default()
	: Super(Id, "Default")
{
}

void MetaTypes::registerNameSpace(MetaNameSpace *ns)
{
	if(!ns) {
		shvError() << "Cannot register NULL namespace";
		return;
	}
	if(!ns->id()) {
		shvError() << "Cannot register namespace with id == 0";
		return;
	}
	RegisteredNamespace &rns = registered_ns[ns->id()];
	rns.metaNameSpace = ns;
}

void MetaTypes::registerType(int ns_id, MetaType *type)
{
	if(!type) {
		shvError() << "Cannot register NULL type";
		return;
	}
	if(!type->id()) {
		shvError() << "Cannot register type with id == 0";
		return;
	}
	RegisteredNamespace &rns = registered_ns[ns_id];
	rns.registeredTypes[type->id()] = type;
}

MetaNameSpace *MetaTypes::registeredNameSpace(int ns_id)
{
	if(registered_ns.find(ns_id) == registered_ns.cend()) {
		return nullptr;
	}
	RegisteredNamespace &rns = registered_ns[ns_id];
	return rns.metaNameSpace;
}

MetaType *MetaTypes::registeredType(int ns_id, int type_id)
{
	auto ns_it = registered_ns.find(ns_id);
	if(ns_it == registered_ns.end())
		return nullptr;
	auto type_it = ns_it->registeredTypes.find(type_id);
	if(type_it == ns_it->registeredTypes.end())
		return nullptr;
	return type_it.value();
}

const char *MetaTypes::metaKeyName(int namespace_id, int type_id, int tag)
{
	switch(tag) {
	case MetaTypes::Tag::MetaTypeId: return "T";
	case MetaTypes::Tag::MetaTypeNameSpaceId: return "S";
	}
	if(namespace_id == MetaTypes::Default::Value) {
		if(type_id == type::RpcMessage::Value /*|| meta_type_id == GlobalMetaTypeId::SkyNetRpcMessage*/) {
			switch(tag) {
			case type::RpcMessage::Tag::RequestId: return "RqId";
			case type::RpcMessage::Tag::RpcCallType: return "Type";
			case type::RpcMessage::Tag::DeviceId: return "DevId";
			}
		}
		/*
		else if(meta_type_id == GlobalMetaTypeId::SkyNetRpcMessage) {
			switch(meta_key_id) {
			case SkyNetRpcMessageMetaKey::DeviceId: return "DeviceId";
			}
		}
		*/
	}
	return "";
}


const char *MetaTypes::metaTypeName(int namespace_id, int type_id)
{
	if(namespace_id == MetaTypes::Default::Value) {
		switch(type_id) {
		case type::RpcMessage::Value: return "Rpc";
		//case GlobalMetaTypeId::SkyNetRpcMessage: return "SkyRpc";
		}
	}
	return "";
}
#endif

static void initMetaTypes()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static ns::Default def;
		MetaTypes::registerNameSpace(ns::Default::ID, &def);
	}
}

void MetaTypes::registerNameSpace(int ns_id, MetaTypes::NameSpace *ns)
{
	initMetaTypes();
	if(ns == nullptr)
		registered_ns.erase(ns_id);
	else
		registered_ns[ns_id] = ns;
}

void MetaTypes::registerTypeId(int ns_id, int type_id, MetaTypes::Type *type)
{
	initMetaTypes();
	auto it = registered_ns.find(ns_id);
	if(it == registered_ns.end())
		SHV_EXCEPTION("Unknown namespace id!");
	if(type == nullptr)
		it->second->m_types.erase(type_id);
	else
		it->second->m_types[type_id] = type;
}

const MetaTypes::NameSpace &MetaTypes::metaNameSpace(int ns_id)
{
	initMetaTypes();
	static NameSpace invalid("");
	auto it = registered_ns.find(ns_id);
	if(it == registered_ns.end())
		return invalid;
	return *(it->second);
}

const MetaTypes::Type &MetaTypes::metaType(int ns_id, int type_id)
{
	static Type invalid("");
	const NameSpace &ns = metaNameSpace(ns_id);
	if(ns.isValid()) {
		auto it = ns.m_types.find(type_id);
		if(it != ns.m_types.end()) {
			return *(it->second);
		}
	}
	return invalid;
}

}}}
