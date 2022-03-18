#include "shvpath.h"
#include "../utils.h"

#include "../log.h"
#include "../stringview.h"

namespace shv {
namespace core {
namespace utils {

//static const std::string DDOT_SLASH("../");
//static const std::string DDOT("..");

const char ShvPath::SHV_PATH_METHOD_DELIM = ':';

bool ShvPath::startsWithPath(const StringView &path, size_t *pos) const
{
	return startsWithPath(*this, path, pos);
}

bool ShvPath::startsWithPath(const StringView &str, const StringView &path, size_t *pos)
{
	auto set_pos = [pos](size_t val, bool ret_val) -> bool {
		if(pos)
			*pos = val;
		return ret_val;
	};
	if(path.empty())
		return set_pos(0, true);
	if(StringView(str).startsWith(path)) {
		if(str.size() == path.size())
			return set_pos(str.size(), true);
		if(str[path.size()] == SHV_PATH_DELIM)
			return set_pos(path.size() + 1, true);
		if(path[path.size() - 1] == SHV_PATH_DELIM) // path contains trailing /
			return set_pos(path.size(), true);
	}
	return set_pos(std::string::npos, false);
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
core::StringViewList ShvPath::splitPath(const shv::core::StringView &shv_path)
{
	return core::StringView{shv_path}.split(SHV_PATH_DELIM, SHV_PATH_QUOTE, core::StringView::SkipEmptyParts, core::StringView::RemoveQuotes);
}

StringView ShvPath::firstDir(const StringView &shv_path, size_t *next_dir_pos)
{
	StringView dir = shv_path.getToken(SHV_PATH_DELIM, SHV_PATH_QUOTE);
	size_t next_pos = dir.size() + 1;
	if(next_pos > shv_path.size())
		next_pos = shv_path.size();
	if(dir.size() >= 2 && dir.at(0) == SHV_PATH_QUOTE && dir.at(dir.size() - 1) == SHV_PATH_QUOTE) {
		dir = dir.mid(1, dir.size() - 2);
	}
	if(next_dir_pos)
		*next_dir_pos = next_pos;
	return dir;
}

StringView ShvPath::takeFirsDir(StringView &shv_path)
{
	size_t next_dir_pos;
	StringView first = firstDir(shv_path, &next_dir_pos);
	shv_path = shv_path.mid(next_dir_pos).normalized();
	return first;
}

ShvPath ShvPath::join(const std::vector<std::string> &shv_path)
{
	ShvPath ret;
	for(const std::string &s : shv_path) {
		if(s.empty())
			continue;
		bool need_quotes = false;
		if(s.find(SHV_PATH_DELIM) != std::string::npos)
			need_quotes = true;
		//shvWarning() << sv.toString() << "~~~" << need_quotes;
		if(!ret.empty())
			ret += SHV_PATH_DELIM;
		if(need_quotes)
			ret += SHV_PATH_QUOTE;
		ret += s;
		if(need_quotes)
			ret += SHV_PATH_QUOTE;
	}
	return ret;
}

ShvPath ShvPath::join(const StringViewList &shv_path)
{
	return join(shv_path.cbegin(), shv_path.cend());
}

static bool need_quotes(const StringView &dir)
{
	if(dir.indexOf(ShvPath::SHV_PATH_DELIM) >= 0) {
		if(dir.size() >= 2 && dir.at(0) == ShvPath::SHV_PATH_QUOTE && dir.at(dir.size() - 1) == ShvPath::SHV_PATH_QUOTE)
			return false;
		return true;
	}
	return false;
}

ShvPath ShvPath::join(std::vector<StringView>::const_iterator first, std::vector<StringView>::const_iterator last)
{
	ShvPath ret;
	for(std::vector<StringView>::const_iterator it = first; it != last; ++it) {
		if(it->empty())
			continue;
		bool quotes = need_quotes(*it);
		//shvWarning() << sv.toString() << "~~~" << need_quotes;
		if(!ret.empty())
			ret += SHV_PATH_DELIM;
		if(quotes)
			ret += SHV_PATH_QUOTE;
		ret += it->toString();
		if(quotes)
			ret += SHV_PATH_QUOTE;
	}
	return ret;
}

ShvPath ShvPath::joinPath(StringView path1, StringView path2)
{
	return shv::core::Utils::joinPath(path1, path2);
}

ShvPath ShvPath::joinPath(StringView path) const
{
	return joinPath(*this, path);
}

ShvPath ShvPath::appendDir(StringView path1, StringView dir)
{
	while(path1.endsWith('/'))
		path1 = path1.mid(0, path1.size() - 1);
	if(dir.empty())
		return path1.toString();
	bool quotes = need_quotes(dir);
	std::string dir_str = dir.toString();
	if(quotes)
		dir_str = SHV_PATH_QUOTE + dir_str + SHV_PATH_QUOTE;
	if(path1.empty())
		return dir_str;
	return path1.toString() + SHV_PATH_DELIM + dir_str;
}

ShvPath ShvPath::appendDir(StringView dir) const
{
	return appendDir(*this, dir);
}
/*
StringView ShvPath::midPath(const std::string &path, size_t start, size_t len)
{
	bool in_quote = false;
	size_t slash_cnt = 0;
	size_t start_ix = 0;
	size_t subpath_len = path.size();
	for(size_t ix = 0; ix < path.size(); ix++) {
		auto c = path[ix];
		if (c == SHV_PATH_DELIM) {
			if(!in_quote) {
				slash_cnt++;
				if(slash_cnt == start) {
					start_ix = ix + 1;
				}
				if(slash_cnt == start + len) {
					subpath_len = ix - start_ix;
					break;
				}
			}
		}
		else if(c == SHV_PATH_QUOTE) {
			in_quote = !in_quote;
		}
	}
	if(slash_cnt == 0 && start > 0)
		return StringView();
	return StringView(path, start_ix, subpath_len);
}
*/
StringViewList ShvPath::split(const StringView &shv_path)
{
	return splitPath(shv_path);
}

bool ShvPath::matchWild(const std::string &pattern) const
{
	const shv::core::StringViewList ptlst = shv::core::StringView(pattern).split(SHV_PATH_DELIM);
	return matchWild(ptlst);
}

bool ShvPath::matchWild(const shv::core::StringViewList &pattern_lst) const
{
	const shv::core::StringViewList path_lst = shv::core::StringView(*this).split(SHV_PATH_DELIM);
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

