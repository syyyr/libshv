#pragma once

#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT MetaMethod
{
public:
	enum class Signature {VoidVoid = 0, VoidParam, RetVoid, RetParam};
	enum class Attribute {
		Signature = 1 << 0,
		IsSignal = 1 << 1,
	};
public:
	MetaMethod(const char *name, Signature ms, bool is_sig)
		: m_name(name)
		, m_signature(ms)
		, m_isSignal(is_sig) {}

	const char *name() const {return m_name;}
	RpcValue attributes(unsigned mask) const
	{
		RpcValue::List lst;
		if(mask & (unsigned)Attribute::Signature)
			lst.push_back((unsigned)m_signature);
		if(mask & (unsigned)Attribute::IsSignal)
			lst.push_back(m_isSignal);
		if(lst.empty())
			return name();
		lst.insert(lst.begin(), name());
		return lst;
	}
private:
	const char *m_name;
	Signature m_signature;
	bool m_isSignal;
};

} // namespace chainpack
} // namespace shv
