#pragma once

#include "shvcoreglobal.h"

#include <limits>
#include <string>
#include <vector>

namespace shv {
namespace core {

class SHVCORE_DECL_EXPORT StringView
{
public:
	enum SplitBehavior {KeepEmptyParts, SkipEmptyParts};

	using StringViewList = std::vector<StringView>;
public:
	StringView();
	StringView(const StringView &strv);
	StringView(const std::string &str, size_t start = 0);
	StringView(const std::string &str, size_t start, size_t len);

	StringView& operator=(const StringView &o);
	bool operator==(const std::string &o) const;
	bool operator==(const StringView &o) const;
	bool operator==(const char *o) const;
	char operator[](size_t ix) const;
	char at(size_t ix) const;
	char at2(int ix) const;

	const std::string &str() const {return *m_str;}
	size_t start() const {return m_start;}
	size_t end() const {return m_start + m_length;}
	void setStart(size_t ix) {m_start = ix;}
	size_t length() const {return m_length;}
	//size_t space() const;
	bool empty() const {return length() == 0;}
	bool valid() const;
	void normalize();
	StringView normalized() const;
	std::string toString() const;

	bool startsWith(const StringView &str) const;

	StringView mid(size_t start)const {return mid(start, (length() > start)? length() - start: 0);}
	StringView mid(size_t start, size_t len) const;

	StringView getToken(char delim = ' ');
	StringViewList split(char delim, SplitBehavior split_behavior = SkipEmptyParts) const;
	static std::string join(StringViewList::const_iterator first, StringViewList::const_iterator last, const char delim);
	static std::string join(const StringViewList &lst, const char delim) { return join(lst.begin(), lst.end(), delim); }
	static std::string join(StringViewList::const_iterator first, StringViewList::const_iterator last, const std::string &delim);
	static std::string join(const StringViewList &lst, const std::string &delim) { return join(lst.begin(), lst.end(), delim); }
private:
	const std::string *m_str;
	size_t m_start;
	size_t m_length;
};

using StringViewList = StringView::StringViewList;

} // namespace core
} // namespace shv
