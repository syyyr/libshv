#include "exception.h"

#include <iostream>
#include <cstdlib>

namespace shv {
namespace chainpack {

bool Exception::s_abortOnException = false;

Exception::Exception(const std::string &_msg, const std::string &_where)
	: m_msg(_msg)
	, m_where(_where)
{
	std::cerr << " SHV_EXCEPTION: " << where() << " " << message() << std::endl;
	if(isAbortOnException())
		std::abort();
}

const char *Exception::what() const noexcept
{
	return m_msg.c_str();
}

} // namespace chainpack
} // namespace shv
