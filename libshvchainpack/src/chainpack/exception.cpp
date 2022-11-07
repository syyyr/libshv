#include "exception.h"

#include <necrolog.h>

#include <cstdlib>

namespace shv::chainpack {

bool Exception::s_abortOnException = false;

Exception::Exception(const std::string &_msg, const std::string &_where)
	: Exception(_msg, _where, nullptr)
{
}

Exception::Exception(const std::string &_msg, const std::string &_where, const char *_log_topic)
	: m_msg(_msg)
	, m_where(_where)
{
	if(!_where.empty() || (_log_topic && *_log_topic)) {
		if(_log_topic && *_log_topic)
			nCError(_log_topic) << "SHV_EXCEPTION:" << where() << message();
		else
			nError() << "SHV_EXCEPTION:" << where() << message();
	}
	if(isAbortOnException())
		std::abort();
}

const char *Exception::what() const noexcept
{
	return m_msg.c_str();
}

void Exception::Exception::setAbortOnException(bool on)
{
	s_abortOnException = on;
}

bool Exception::Exception::isAbortOnException()
{
	return s_abortOnException;
}

} // namespace shv
