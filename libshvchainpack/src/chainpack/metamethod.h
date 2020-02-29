#pragma once

#include "rpcvalue.h"
#include "rpc.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT MetaMethod
{
public:
	enum class Signature {VoidVoid = 0, VoidParam, RetVoid, RetParam};
	struct Flag {
		enum {
			None = 0,
			IsSignal = 1 << 0,
			IsGetter = 1 << 1,
			IsSetter = 1 << 2,
			LargeResultHint = 1 << 3,
			IsDiscrete = 1 << 4,
		};
	};
	struct AccessLevel {
		enum {
			None = 0,
			Browse = 1,
			Read = 10,
			Write = 20,
			Command = 30,
			Config = 40,
			Service = 50,
			Devel = 60,
			Admin = 70,
		};
	};
	struct DirAttribute {
		enum {
			Signature = 1 << 0,
			Flags = 1 << 1,
			//AccessLevel = 1 << 2,
			AccessGrant = 1 << 2,
		};
	};
	struct LsAttribute {
		enum {
			HasChildren = 1 << 0,
		};
	};
public:
	MetaMethod() {}
	MetaMethod(std::string name, Signature ms, unsigned flags = 0, const shv::chainpack::RpcValue &access_grant = shv::chainpack::Rpc::ROLE_BROWSE)
	    : m_name(std::move(name))
	    , m_signature(ms)
	    , m_flags(flags)
	    //, m_accessLevel(access_level)
	    , m_accessGrant(access_grant)
	{}

	//static constexpr bool IsSignal = true;

	const std::string& name() const {return m_name;}
	Signature signature() const {return m_signature;}
	unsigned flags() const {return m_flags;}
	const shv::chainpack::RpcValue& accessGrant() const {return m_accessGrant;}
	RpcValue attributes(unsigned mask) const
	{
		RpcValue::List lst;
		if(mask & (unsigned)DirAttribute::Signature)
			lst.push_back((unsigned)m_signature);
		if(mask & DirAttribute::Flags)
			lst.push_back(m_flags);
		//if(mask & DirAttribute::AccessLevel)
		//	lst.push_back(m_accessLevel);
		if(mask & DirAttribute::AccessGrant)
			lst.push_back(m_accessGrant);
		if(lst.empty())
			return name();
		lst.insert(lst.begin(), name());
		return RpcValue{lst};
	}

	static Signature signatureFromString(const std::string &sigstr);
private:
	std::string m_name;
	Signature m_signature = Signature::VoidVoid;
	unsigned m_flags = 0;
	shv::chainpack::RpcValue m_accessGrant;
};

} // namespace chainpack
} // namespace shv
