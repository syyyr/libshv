#include "stringview.h"

namespace shv {
namespace core {

static std::string empty_string;

StringView::StringView()
	: StringView(empty_string)
{
}

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

bool StringView::operator==(const char *o) const
{
	size_t i;
	for (i = 0; i < length(); ++i) {
		char c = o[i];
		if(!c)
			return false;
		if((*this)[i] != c)
			return false;
	}
	return o[i] == 0;
}

char StringView::operator[](size_t ix) const
{
	return str()[m_start + ix];
}

char StringView::at(size_t ix) const
{
	return m_str->at(m_start + ix);
}

char StringView::value(int ix) const
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

StringView StringView::getToken(char delim, char quote)
{
	if(empty())
		return *this;
	bool in_quotes = false;
	for (size_t i = 0; i < length(); ++i) {
		char c = at(i);
		if(quote) {
			if(c == quote) {
				in_quotes = !in_quotes;
				continue;
			}
		}
		if(c == delim && !in_quotes)
			return mid(0, i);
	}
	return *this;
}

std::vector<StringView> StringView::split(char delim, char quote, StringView::SplitBehavior split_behavior) const
{
	using namespace std;
	vector<StringView> ret;
	if(empty())
		return ret;
	StringView strv(*this);
	while(true) {
		StringView token = strv.getToken(delim, quote);
		if(split_behavior == KeepEmptyParts || token.length())
			ret.push_back(token);
		if(token.end() >= strv.end())
			break;
		strv = strv.mid(token.length() + 1);
	}
	return ret;
}

std::string StringView::join(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last, const char delim)
{
	std::string ret;
	int i = 0;
	while(first != last) {
		if(i++ > 0)
			ret += delim;
		ret += first->toString();
		first++;
	}
	return ret;
}

std::string StringView::join(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last, const std::string &delim)
{
	std::string ret;
	int i = 0;
	while(first != last) {
		if(i++ > 0)
			ret += delim;
		ret += first->toString();
		first++;
	}
	return ret;
}

StringView StringViewList::value(int ix) const
{
	if(ix < 0)
		ix = static_cast<int>(size()) + ix;
	if(ix < 0 || ix >= static_cast<int>(size()))
		return StringView();
	return this->at(static_cast<size_t>(ix));
}

StringViewList StringViewList::mid(long start, long len) const
{
	return StringViewList(begin() + start, begin() + start + len);
}

bool StringViewList::startsWith(const StringViewList &lst) const
{
	size_t i;
	for (i = 0; i < size() && i < lst.size(); ++i) {
		if(!(this->at(i) == lst.at(i)))
			return false;
	}
	return i == lst.size();
}

} // namespace core
} // namespace shv
