#include "abstractstreamtokenizer.h"

namespace shv {
namespace chainpack {

AbstractStreamTokenizer::AbstractStreamTokenizer(std::istream &in)
	: m_in(in)
{
}

void AbstractStreamTokenizer::unget()
{
	m_in.unget();
}

void AbstractStreamTokenizer::pushContext()
{
	m_contextStack.push_back(m_context);
	m_context = Context();
}

void AbstractStreamTokenizer::popContext()
{
	if(m_contextStack.empty()) {
		m_context = Context();
	}
	else {
		m_context = m_contextStack.back();
		m_contextStack.pop_back();
	}
}

} // namespace chainpack
} // namespace shv
