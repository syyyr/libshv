#pragma once

#include <cstddef>
#include <map>

namespace shv {
namespace core {
namespace chainpack {
namespace meta {

struct Tag {
	enum Enum {
		Invalid = -1,
		MetaTypeId = 1,
		MetaTypeNameSpaceId,
		//MetaIMap,
		//MetaTypeName,
		//MetaTypeNameSpaceName,
		USER = 8
	};
};
class MetaInfo
{
public:
	int id = 0;
	const char *name = nullptr;

	MetaInfo() : id(0), name(nullptr) {}
	MetaInfo(int id, const char *name) : id(id), name(name) {}

	bool isValid() const {return (name && name[0]);}
};
class MetaType;
class MetaNameSpace
{
public:
	MetaNameSpace(const char *name = nullptr) : m_name(name) {}
	const char *name() const {return m_name;}
	bool isValid() const {return (m_name && m_name[0]);}

	std::map<int, MetaType*>& types() {return m_types;}
	const std::map<int, MetaType*>& types() const {return m_types;}
protected:
	const char *m_name;
	std::map<int, MetaType*> m_types;
};
class MetaType
{
public:
	MetaType(const char *name) : m_name(name) {}
	const char *name() const {return m_name;}
	const MetaInfo& tagById(int id) const;
	const MetaInfo& keyById(int id) const;
	bool isValid() const {return (m_name && m_name[0]);}
protected:
	const char *m_name;
	std::map<int, MetaInfo> m_tags;
	std::map<int, MetaInfo> m_keys;
};

void registerNameSpace(int ns_id, MetaNameSpace *ns);
void registerType(int ns_id, int type_id, MetaType *tid);
const MetaNameSpace& registeredNameSpace(int ns_id);
const MetaType& registeredType(int ns_id, int type_id);

class DefaultNS : public meta::MetaNameSpace
{
	using Super = meta::MetaNameSpace;
public:
	enum {ID = 0};
	DefaultNS();
};

}}}}
