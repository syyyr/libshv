#include "metamethod.h"

namespace shv {
namespace chainpack {

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


} // namespace chainpack
} // namespace shv
