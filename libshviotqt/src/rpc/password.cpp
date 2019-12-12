#include "password.h"

#include <shv/core/string.h>

namespace shv {
namespace iotqt {
namespace rpc {

const char* Password::formatToString(Password::Format f)
{
	switch(f) {
	case Format::Plain: return "PLAIN";
	case Format::Sha1: return "SHA1";
	default: return "INVALID";
	}
}

Password::Format Password::formatFromString(const std::string &s)
{
	std::string typestr = shv::core::String::toUpper(s);
	if(typestr == formatToString(Format::Plain))
		return Format::Plain;
	if(typestr == formatToString(Format::Sha1))
		return Format::Sha1;
	return Format::Invalid;
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
