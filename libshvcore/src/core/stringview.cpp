#include "stringview.h"

namespace shv {
namespace core {

StringView::StringView(const StringView &strv)
	: m_str(strv.m_str)
	, m_start(strv.start())
	, m_length(strv.length())
{
}

StringView::StringView(const std::string &str, size_t start)
	: StringView(str, start, str.length() - start)
{
}

StringView::StringView(const std::string &str, size_t start, size_t len)
	: m_str(&str)
	, m_start(start)
	, m_length(len)
{
	normalize();
}

StringView &StringView::operator=(const StringView &o)
{
	m_str = o.m_str;
	m_start = o.m_start;
	m_length = o.m_length;
	return *this;
}

bool StringView::operator==(const std::string &o) const
{
	return this->operator ==(StringView(o));
}

bool StringView::operator==(const StringView &o) const
{
	if(length() != o.length())
		return false;
	for (size_t i = 0; i < length() && i < o.length(); ++i) {
		if((*this)[i] != o[i])
			return false;
	}
	return true;
}

char StringView::operator[](size_t ix) const
{
	return str()[m_start + ix];
}

char StringView::at(size_t ix) const
{
	return m_str->at(m_start + ix);
}

char StringView::at2(int ix) const
{
	if(ix < 0)
		ix = m_length + ix;
	if(ix < 0)
		return 0;
	if((unsigned)ix >= m_length)
		return 0;
	return str()[m_start + ix];
}
/*
size_t StringView::space() const
{
	if(m_start + m_length >= m_str->length())
		return 0;
	return m_str->length() - (m_start + m_length);
}
*/
bool StringView::valid() const
{
	if(m_length == 0)
		return true;
	return m_start < m_str->length() && (m_start + m_length) <= m_str->length();
}

void StringView::normalize()
{
	if(m_start >= m_str->length()) {
		m_start = m_str->length();
		m_length = 0;
	}
	else {
		if(m_start + m_length > m_str->length()) {
			m_length = m_str->length() - m_start;
		}
	}
}

StringView StringView::normalized() const
{
	StringView ret = *this;
	ret.normalize();
	return ret;
}

std::string StringView::toString() const
{
	return m_str->substr(m_start, m_length);
}

bool StringView::startsWith(const StringView &str) const
{
	size_t i;
	for (i = 0; i < length() && i < str.length(); ++i) {
		if((*this)[i] != str[i])
			return false;
	}
	return i == str.length();
}

StringView StringView::mid(size_t start, size_t len) const
{
	return StringView(str(), this->start() + start, len);
}

StringView StringView::getToken(char delim)
{
	if(empty())
		return *this;
	const std::string &s = str();
	size_t pos = s.find_first_of(delim, start());
	if(pos != std::string::npos) {
		return mid(0, pos - start());
	}
	return *this;
}

std::vector<StringView> StringView::split(char delim, StringView::SplitBehavior split_behavior) const
{
	using namespace std;
	vector<StringView> ret;
	if(empty())
		return ret;
	StringView strv(*this);
	while(true) {
		StringView token = strv.getToken(delim);
		if(split_behavior == KeepEmptyParts || token.length())
			ret.push_back(token);
		if(token.end() >= strv.end())
			break;
		strv = strv.mid(token.length() + 1);
	}
	return ret;
}

} // namespace core
} // namespace shv
