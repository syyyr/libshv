#include "aclpassword.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

static bool str_eq(const std::string &s1, const char *s2)
{
	size_t i;
	for (i = 0; i < s1.size(); ++i) {
		char c2 = s2[i];
		if(!c2)
			return false;
		if(toupper(c2 != toupper(s1[i])))
			return false;
	}
	return s2[i] == 0;
}

RpcValue AclPassword::toRpcValueMap() const
{
	return RpcValue::Map {
		{"password", password},
		{"format", formatToString(format)},
	};
}

AclPassword AclPassword::fromRpcValue(const RpcValue &v)
{
	AclPassword ret;
	if(v.isMap()) {
		const auto &m = v.toMap();
		ret.password = m.value("password").toString();
		ret.format = formatFromString(m.value("format").toString());
	}
	return ret;
}

const char* AclPassword::formatToString(AclPassword::Format f)
{
	switch(f) {
	case Format::Plain: return "PLAIN";
	case Format::Sha1: return "SHA1";
	default: return "INVALID";
	}
}

AclPassword::Format AclPassword::formatFromString(const std::string &s)
{
	if(str_eq(s, formatToString(Format::Plain)))
		return Format::Plain;
	if(str_eq(s, formatToString(Format::Sha1)))
		return Format::Sha1;
	return Format::Invalid;
}

} // namespace chainpack
} // namespace shv
