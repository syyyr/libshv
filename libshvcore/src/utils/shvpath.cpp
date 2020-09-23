#include "shvpath.h"

#include "../log.h"
#include "../stringview.h"

namespace shv {
namespace core {
namespace utils {

//static const std::string DDOT_SLASH("../");
//static const std::string DDOT("..");

const char ShvPath::SHV_PATH_METHOD_DELIM = ':';

bool ShvPath::startsWithPath(const std::string &path, size_t *pos) const
{
	return startsWithPath(*this, path, pos);
}

bool ShvPath::startsWithPath(const std::string &str, const std::string &path, size_t *pos)
{
	auto set_pos = [pos](size_t val, bool ret_val) -> bool {
		if(pos)
			*pos = val;
		return ret_val;
	};
	if(path.empty())
		return set_pos(0, true);
	if(startsWith(str, path)) {
		if(str.size() == path.size())
			return set_pos(str.size(), true);
		if(str[path.size()] == '/')
			return set_pos(path.size() + 1, true);
		if(path[path.size() - 1] == '/') // path contains trailing /
			return set_pos(path.size(), true);
	}
	return set_pos(std::string::npos, false);
}

size_t ShvPath::serviceProviderMarkIndex(const std::string &path)
{
	size_t ix = 0;
	while(ix < path.size()) {
		if(path[ix] == '/')
			ix++;
		else
			break;
	}
	if(ix > 1 && ix < path.size() && (path[ix - 1] == SERVICE_PROVIDER_MARK))
		return ix - 1;
	return 0;
}
/*
core::StringViewList ShvPath::cleanPath(const core::StringViewList &path_list)
{
	core::StringViewList ret;
	size_t ret_len = 0;
	for (size_t i = 0; i < path_list.size(); ++i) {
		const core::StringView &sv = path_list[i];
		if(sv == DDOT && (ret_len > 0) && !(ret[ret_len - 1] == DDOT)) {
				ret_len--;
		}
		else {
			if(ret_len < ret.size()) {
				ret[ret_len] = sv;
			}
			else {
				ret.push_back(sv);
			}
			ret_len++;
		}
	}
	ret.resize(ret_len);
	//shvWarning() << "clean path:" << path_list.join('/') << "--->" << ret.join('/');
	return ret;
}

std::string ShvPath::cleanPath(const std::string &path)
{
	return cleanPath(split(path)).join('/');
}

ShvPath ShvPath::joinAndClean(const std::string &path1, const std::string &path2)
{
	return cleanPath(path1 + '/' + path2);
}
*/
core::StringViewList ShvPath::split() const
{
	return split(*this);
}

core::StringViewList ShvPath::split(const std::string &shv_path)
{
	return core::StringView{shv_path}.split(SHV_PATH_DELIM, SHV_PATH_QUOTE, core::StringView::SkipEmptyParts);
}

ShvPath ShvPath::join(const core::StringViewList &shv_path)
{
	ShvPath ret;
	for(const core::StringView &sv : shv_path) {
		bool need_quotes = false;
		if(sv.indexOf(SHV_PATH_DELIM) >= 0)
			need_quotes = true;
		//shvWarning() << sv.toString() << "~~~" << need_quotes;
		if(!ret.empty())
			ret += SHV_PATH_DELIM;
		if(need_quotes)
			ret += SHV_PATH_QUOTE;
		ret += sv.toString();
		if(need_quotes)
			ret += SHV_PATH_QUOTE;
	}
	/*
	shvWarning() << std::accumulate(shv_path.begin(), shv_path.end(), std::string(),
									[](const core::StringView& a, const core::StringView& b) -> std::string {
										return a.toString() + (a.length() > 0 ? "/" : "") + b.toString();
									} )
				 << "--->" << ret;
	*/
	return ret;
}

ShvPath ShvPath::join(const std::string &path1, const std::string &path2)
{
	ShvPath ret = path1;
	while(core::String::endsWith(ret, '/'))
		ret = ret.substr(0, ret.size() - 1);
	size_t ix = 0;
	for(; ix < path2.size(); ix++)
		if(path2[ix] != '/')
			break;
	if(!ret.empty() && ix < path2.size())
		ret += '/';
	ret += path2.substr(ix);
	//shvWarning() << path1 << "+" << path2 << "--->" << ret;
	return ret;
}
/*
ShvPath ShvPath::join(const std::string &path1, const std::string &path2)
{
	return join({path1, path2});
}

ShvPath ShvPath::join(const std::vector<const std::string &> &paths)
{
	ShvPath ret;
	for(const std::string &sv : paths) {
		bool need_quotes = false;
		if(indexOf(sv, SHV_PATH_DELIM) != std::string::npos) {
			need_quotes = true;
			break;
		}
		if(!ret.empty())
			ret += SHV_PATH_DELIM;
		if(need_quotes)
			ret += SHV_PATH_QUOTE;
		ret += sv;
		if(need_quotes)
			ret += SHV_PATH_QUOTE;
	}
	return ret;
}
*/
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

