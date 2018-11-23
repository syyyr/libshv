#include "shvpath.h"

#include <shv/core/stringview.h>

namespace shv {
namespace iotqt {
namespace utils {

bool ShvPath::startsWithPath(const std::string &path) const
{
	if(startsWith(path)) {
		if(size() == path.size())
			return true;
		return (*this)[path.size()] == '/';
	}
	return false;
}

bool ShvPath::matchWild(const std::string &pattern) const
{
	const shv::core::StringViewList ptlst = shv::core::StringView(pattern).split('/');
	return matchWild(ptlst);
}

bool ShvPath::matchWild(const shv::core::StringViewList &pattern_lst) const
{
	const shv::core::StringViewList path_lst = shv::core::StringView(*this).split('/');
	return matchWild(path_lst, pattern_lst);
}

bool ShvPath::matchWild(const core::StringViewList &path_lst, const core::StringViewList &pattern_lst)
{
	size_t ptix = 0;
	size_t phix = 0;
	while(true) {
		if(phix == path_lst.size() && ptix == pattern_lst.size())
			return true;
		if(ptix == pattern_lst.size() && phix < path_lst.size())
			return false;
		if(phix == path_lst.size() && ptix == pattern_lst.size() - 1 && pattern_lst[ptix] == "**")
			return true;
		if(phix == path_lst.size() && ptix < pattern_lst.size())
			return false;
		const shv::core::StringView &pt = pattern_lst[ptix];
		if(pt == "*") {
			// match exactly one path segment
		}
		else if(pt == "**") {
			// match zero or more path segments
			ptix++;
			if(ptix == pattern_lst.size())
				return true;
			const shv::core::StringView &pt2 = pattern_lst[ptix];
			do {
				const shv::core::StringView &ph = path_lst[phix];
				if(ph == pt2)
					break;
				phix++;
			} while(phix < path_lst.size());
			if(phix == path_lst.size())
				return false;
		}
		else {
			const shv::core::StringView &ph = path_lst[phix];
			if(!(ph == pt))
				return false;
		}
		ptix++;
		phix++;
	}
}

}}}

