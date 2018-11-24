#pragma once

#include "../shviotqtglobal.h"

#include <shv/core/string.h>

namespace shv {

namespace core { class StringViewList; }

namespace iotqt {
namespace utils {

class SHVIOTQT_DECL_EXPORT ShvPath : public shv::core::String
{
	using Super = shv::core::String;
public:
	ShvPath(shv::core::String &&o) : Super(std::move(o)) {}
	ShvPath(const shv::core::String &o) : Super(o) {}
	using Super::Super;

	bool startsWithPath(const std::string &path) const;
	static bool startsWithPath(const std::string &str, const std::string &path);

	bool matchWild(const std::string &pattern) const;
	bool matchWild(const shv::core::StringViewList &pattern_lst) const;
	static bool matchWild(const shv::core::StringViewList &path_lst, const shv::core::StringViewList &pattern_lst);
};

}}}

