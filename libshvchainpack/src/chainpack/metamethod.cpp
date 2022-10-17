#include "metamethod.h"

namespace shv {
namespace chainpack {

RpcValue MetaMethod::toRpcValue() const
{
	RpcValue::Map ret;
	ret["name"] = m_name;
	ret["signature"] = static_cast<int>(m_signature);
	ret["flags"] = m_flags;
	ret["accessGrant"] = m_accessGrant;
	if(!m_description.empty())
		ret["description"] = m_description;
	RpcValue::Map tags = m_tags;
	ret.merge(tags);
	return ret;
}

MetaMethod MetaMethod::fromRpcValue(const RpcValue &rv)
{
	MetaMethod ret;
	RpcValue::Map map = rv.asMap();
	RpcValue::Map tags = map.take("tags").asMap();
	map.merge(tags);
	ret.m_name = map.take("name").asString();
	ret.m_signature = static_cast<Signature>(map.take("signature").toInt());
	ret.m_flags = map.take("flags").toUInt();
	ret.m_accessGrant = map.take("accessGrant");
	ret.m_description = map.take("description").asString();
	ret.m_tags = map;
	return ret;
}

MetaMethod::Signature MetaMethod::signatureFromString(const std::string &sigstr)
{
	if(sigstr == "VoidParam") return Signature::VoidParam;
	if(sigstr == "RetVoid") return Signature::RetVoid;
	if(sigstr == "RetParam") return Signature::RetParam;
	return Signature::VoidVoid;
}

const char *MetaMethod::signatureToString(MetaMethod::Signature sig)
{
	switch(sig) {
	case Signature::VoidVoid: return "VoidVoid";
	case Signature::VoidParam: return "VoidParam";
	case Signature::RetVoid: return "RetVoid";
	case Signature::RetParam: return "RetParam";
	}
	return "";
}

std::string MetaMethod::flagsToString(unsigned flags)
{
	std::string ret;
	auto add_str = [&ret](const char *str) {
		if(ret.empty()) {
			ret = str;
		}
		else {
			ret += ',';
			ret += str;
		}
		return ret;
	};
	if(flags & Flag::IsGetter)
		add_str("Getter");
	if(flags & Flag::IsSetter)
		add_str("Setter");
	if(flags & Flag::IsSignal)
		add_str("Signal");
	return ret;
}


} // namespace chainpack
} // namespace shv
