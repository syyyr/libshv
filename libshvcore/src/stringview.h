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
	char value(int ix) const;

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

	StringView getToken(char delim = ' ', char quote = '\0');
	std::vector<StringView> split(char delim, char quote, SplitBehavior split_behavior = SkipEmptyParts) const;
	std::vector<StringView> split(char delim, SplitBehavior split_behavior = SkipEmptyParts) const { return  split(delim, '\0', split_behavior); }

	static std::string join(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last, const char delim);
	static std::string join(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last, const std::string &delim);
private:
	const std::string *m_str;
	size_t m_start;
	size_t m_length;
};

class SHVCORE_DECL_EXPORT StringViewList : public std::vector<StringView>
{
	using Super = std::vector<StringView>;
public:
	using Super::Super;
	StringViewList() {}
	StringViewList(const std::vector<StringView> &o) : Super(o) {}

	StringViewList mid(long start)const {return mid(start, (length() > start)? length() - start: 0);}
	StringViewList mid(long start, long len) const;

	std::string join(const char delim) const { return StringView::join(begin(), end(), delim); }
	std::string join(const std::string &delim) const { return StringView::join(begin(), end(), delim); }

	bool startsWith(const StringViewList &lst) const;
	long length() const {return (long)size();}
};

} // namespace core
} // namespace shv
