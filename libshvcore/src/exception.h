#pragma once

#include "shvcoreglobal.h"
#include "utils.h"

#include <stdexcept>
#include <string>

#include <shv/chainpack/exception.h>

namespace shv {
namespace core {

class SHVCORE_DECL_EXPORT Exception : public shv::chainpack::Exception
{
	using Super = shv::chainpack::Exception;
public:
	using Super::Super;
};

}}

#define SHV_EXCEPTION(e) throw shv::core::Exception(e, std::string(__FILE__) + ":" + shv::core::Utils::toString(__LINE__))
/*
#define SHV_EXCEPTION(e) { \
	throw std::runtime_error(std::string(__FILE__) + ":" + shv::core::Utils::toString(__LINE__) + " SHV_EXCEPTION: " + e); \
	}
*/
