#pragma once

#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT MetaMethod
{
public:
	enum class Signature {VoidVoid = 0, VoidParam, RetVoid, RetParam};
	struct Flag {
		enum {
			IsSignal = 1 << 0,
		};
	};
	struct AccessLevel {
		enum {
			Host = 0,
			Read = 10,
			Write = 20,
			Command = 30,
			Config = 40,
			Service = 50,
			Admin = 60,
		};
	};
	struct DirAttribute {
		enum {
			Signature = 1 << 0,
			Flags = 1 << 1,
			AccessLevel = 1 << 2,
		};
	};
	struct LsAttribute {
		enum {
			HasChildren = 1 << 0,
		};
	};
public:
	MetaMethod(const char *name, Signature ms, unsigned flags, int access_level = AccessLevel::Host)
		: m_name(name)
		, m_signature(ms)
		, m_flags(flags)
		, m_accessLevel(access_level) {}

	//static constexpr bool IsSignal = true;

	const char *name() const {return m_name;}
	RpcValue attributes(unsigned mask) const
	{
		RpcValue::List lst;
		if(mask & (unsigned)DirAttribute::Signature)
			lst.push_back((unsigned)m_signature);
		if(mask & DirAttribute::Flags)
			lst.push_back(m_flags);
		if(mask & DirAttribute::AccessLevel)
			lst.push_back(m_accessLevel);
		if(lst.empty())
			return name();
		lst.insert(lst.begin(), name());
		return lst;
	}
private:
	const char *m_name;
	Signature m_signature;
	unsigned m_flags;
	int m_accessLevel;
};

} // namespace chainpack
} // namespace shv
