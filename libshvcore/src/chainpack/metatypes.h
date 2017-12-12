#pragma once

#include <cstddef>
#include <map>

namespace shv {
namespace core {
namespace chainpack {

class MetaTypes
{
public:
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
	class Type;
	class NameSpace
	{
		friend class MetaTypes;
	public:
		NameSpace(const char *name = nullptr) : m_name(name) {}
		const char *name() const {return m_name;}
		bool isValid() const {return (m_name && m_name[0]);}
	protected:
		const char *m_name;
		std::map<int, Type*> m_types;
	};
	class Type
	{
		friend class MetaTypes;
	public:
		Type(const char *name) : m_name(name) {}
		const char *name() const {return m_name;}
		const MetaInfo& tagById(int id) const;
		const MetaInfo& keyById(int id) const;
		bool isValid() const {return (m_name && m_name[0]);}
	protected:
		const char *m_name;
		std::map<int, MetaInfo> m_tags;
		std::map<int, MetaInfo> m_keys;
	};
	static void registerNameSpace(int ns_id, NameSpace *ns);
	static void registerTypeId(int ns_id, int type_id, Type *tid);
	static const NameSpace& metaNameSpace(int ns_id);
	static const Type& metaType(int ns_id, int type_id);
};

namespace ns {

class Default : public MetaTypes::NameSpace
{
	using Super = MetaTypes::NameSpace;
public:
	enum {ID = 0};
	Default();
};

}

namespace tid {

//class ChainPackRpc : public MetaTypes::TypeId<1> {};

}

}}}
