#include "exception.h"

#include <necrolog.h>

#include <cstdlib>

namespace shv {
namespace chainpack {

bool Exception::s_abortOnException = false;

Exception::Exception(const std::string &_msg, const std::string &_where)
	: m_msg(_msg)
	, m_where(_where)
{
	nError() << "SHV_CHAINPACK_EXCEPTION:" << where() << message();
	if(isAbortOnException())
		std::abort();
}

const char *Exception::what() const noexcept
{
	return m_msg.c_str();
}

} // namespace chainpack
} // namespace shv
