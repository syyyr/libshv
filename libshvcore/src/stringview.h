#pragma once

#include "shvcoreglobal.h"

#include <limits>
#include <string>
#include <vector>

namespace shv {
namespace core {

using StringView = std::string_view;
class SHVCORE_DECL_EXPORT StringViewList : public std::vector<StringView>
{
	using Super = std::vector<StringView>;
public:
	using Super::Super;
	StringViewList() = default;
	StringViewList(const std::vector<StringView> &o) : Super(o) {}

	StringView value(ssize_t ix) const;
	ssize_t indexOf(const std::string &str) const;

	StringViewList mid(size_t start) const {return mid(start, size());}
	StringViewList mid(size_t start, size_t len) const;

	std::string join(const char delim) const;
	std::string join(const std::string &delim) const;

	bool startsWith(const StringViewList &lst) const;
	ssize_t length() const {return static_cast<ssize_t>(size());}
};

} // namespace core
} // namespace shv
