#include "stringview.h"
#include "string.h"

namespace shv::core {

static const std::string empty_string;

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
	: StringView(str, start, str.size())
{
}

StringView::StringView(const std::string &str, size_t start, size_t len)
	: m_str(&str)
	, m_start(start)
	, m_length(len)
{
	normalize();
}

StringView &StringView::operator=(const StringView &o) = default;

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
		ix = static_cast<int>(m_length - ix);
	if(ix < 0)
		return 0;
	if(static_cast<unsigned>(ix) >= m_length)
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

bool StringView::startsWith(char c) const
{
	return value(0) == c;
}

bool StringView::endsWith(const StringView &str) const
{
	size_t i;
	size_t len1 = length();
	size_t len2 = str.length();
	for (i = 0; i < len1 && i < len2; ++i) {
		if((*this)[len1 - 1 - i] != str[len2 - 1 - i])
			return false;
	}
	return i == len2;
}

bool StringView::endsWith(char c) const
{
	return value(-1) == c;
}

ssize_t StringView::indexOf(char c) const
{
	for (size_t i = 0; i < length(); i++)
		if(at(i) == c)
			return static_cast<ssize_t>(i);
	return -1;
}

ssize_t StringView::lastIndexOf(char c) const
{
	for (ssize_t i = static_cast<ssize_t>(length()) - 1; i >= 0; i--)
		if(at(static_cast<size_t>(i)) == c)
			return i;
	return -1;
}

StringView StringView::mid(size_t start, size_t len) const
{
	//shvWarning() << toString() << start << len;
	return StringView(str(), this->start() + start, len);
}

StringView StringView::slice(int start, int end) const
{
	int s = static_cast<int>(this->start()) + start;
	if(s < 0)
		s = 0;
	if(s > static_cast<int>(m_str->length()))
		s = static_cast<int>(m_str->length());
	int e = static_cast<int>(this->start() + length()) + end;
	if(e < 0)
		e = 0;
	if(e > static_cast<int>(m_str->length()))
		e = static_cast<int>(m_str->length());
	if(e < s)
		e = s;
	return StringView(str(), static_cast<size_t>(s), static_cast<size_t>(e - s));
}

StringView StringView::getToken(char delim, char quote) const
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

std::vector<StringView> StringView::split(char delim, char quote, StringView::SplitBehavior split_behavior, QuoteBehavior quotes_behavior) const
{
	using namespace std;
	vector<StringView> ret;
	if(empty())
		return ret;
	StringView strv(*this);
	while(true) {
		StringView token = strv.getToken(delim, quote);
		if(split_behavior == KeepEmptyParts || token.length()) {
			if(quotes_behavior == RemoveQuotes && token.size() >= 2 && token.at(0) == quote && token.at(token.size() - 1) == quote) {
				ret.push_back(token.mid(1, token.size() - 2));
			}
			else {
				ret.push_back(token);
			}
		}
		if(token.end() >= strv.end())
			break;
		strv = strv.mid(token.length() + 1);
	}
	return ret;
}

int StringView::toInt(bool *ok) const
{
	return shv::core::String::toInt(toString(), ok);
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

StringView StringViewList::value(ssize_t ix) const
{
	if(ix < 0)
		ix = length() + ix;
	if(ix < 0 || ix >= length())
		return StringView();
	return at(static_cast<size_t>(ix));
}

ssize_t StringViewList::indexOf(const std::string &str) const
{
	for (ssize_t i = 0; i < length(); ++i) {
		const StringView &sv = at(static_cast<size_t>(i));
		if(sv == str)
			return i;
	}
	return -1;
}

StringViewList StringViewList::mid(size_t start, size_t len) const
{
	auto end = (start + len) > size()? size(): start + len;
	return StringViewList(begin() + static_cast<long>(start), begin() + static_cast<long>(end));
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

} // namespace shv
