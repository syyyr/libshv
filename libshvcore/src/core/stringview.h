#pragma once

#include "../shvcoreglobal.h"

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
	std::vector<StringView> split(char delim, SplitBehavior split_behavior = SkipEmptyParts) const;
	static std::string join(const std::vector<StringView> &lst, const std::string &delim);
private:
	const std::string *m_str;
	size_t m_start;
	size_t m_length;
};

} // namespace core
} // namespace shv
