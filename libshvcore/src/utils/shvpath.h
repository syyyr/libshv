#pragma once

#include "../shvcoreglobal.h"

#include "../string.h"

namespace shv {
namespace core {
class StringViewList;
class StringView;
namespace utils {

class SHVCORE_DECL_EXPORT ShvPath : public shv::core::String
{
	using Super = shv::core::String;
public:
	static constexpr char SHV_PATH_QUOTE = '\'';
	static constexpr char SHV_PATH_DELIM = '/';
	static const char SHV_PATH_METHOD_DELIM;
public:
	ShvPath() : Super() {}
	ShvPath(shv::core::String &&o) : Super(std::move(o)) {}
	ShvPath(const shv::core::String &o) : Super(o) {}
	using Super::Super;

	const std::string &asString() const { return *this; }

	bool startsWithPath(const StringView &path, size_t *pos = nullptr) const;
	static bool startsWithPath(const StringView &str, const StringView &path, size_t *pos = nullptr);

	static ShvPath join(const std::vector<std::string> &shv_path);
	static ShvPath join(const shv::core::StringViewList &shv_path);
	static ShvPath join(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last);

	static ShvPath joinPath(StringView path1, StringView path2);
	static ShvPath appendDir(StringView path1, StringView dir);

	//static StringView midPath(const std::string &path, size_t start, size_t len = std::numeric_limits<size_t>::max());

	// very dangerous, consider ShvPath("a/b").split() will return dangling references
	// shv::core::StringViewList split() const;

	// split is deprecated, use splitPath instead
	static shv::core::StringViewList split(const shv::core::StringView &shv_path);
	static shv::core::StringViewList splitPath(const shv::core::StringView &shv_path);
	static shv::core::StringView firstDir(const shv::core::StringView &shv_path, size_t *next_dir_pos = nullptr);
	static shv::core::StringView takeFirsDir(shv::core::StringView &shv_path);

	bool matchWild(const std::string &pattern) const;
	bool matchWild(const shv::core::StringViewList &pattern_lst) const;
	static bool matchWild(const shv::core::StringViewList &path_lst, const shv::core::StringViewList &pattern_lst);
};

}}}

