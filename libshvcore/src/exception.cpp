#include "exception.h"
#include "log.h"

#include <iostream>
#include <cstdlib>

bool shv::core::Exception::s_abortOnException = false;

shv::core::Exception::Exception(const std::string &_msg, const std::string &_where)
	: m_msg(_msg)
	, m_where(_where)
{
	shvError() << "SHV_EXCEPTION:" << where() << message();
	if(isAbortOnException())
		std::abort();
}

const char *shv::core::Exception::what() const noexcept
{
	return m_msg.c_str();
}
