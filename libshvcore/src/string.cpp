#include "string.h"
#include "stringview.h"

#include <algorithm>

namespace shv::core {

const char* String::WhiteSpaceChars = " \t\n\r\f\v";

std::string::size_type String::indexOf(const std::string & str_haystack, const std::string &str_needle, String::CaseSensitivity case_sensitivity)
{
	auto it = std::search(
				  str_haystack.begin(), str_haystack.end(),
				  str_needle.begin(), str_needle.end(),
				  (case_sensitivity == CaseInsensitive)
					? [](char a, char b) { return std::tolower(a) == std::tolower(b); }
					: [](char a, char b) { return a == b;}
	);
	return (it == str_haystack.end())? std::string::npos: static_cast<std::string::size_type>(it - str_haystack.begin());
}

std::string::size_type String::indexOf(const std::string &haystack, char needle)
{
	for (std::string::size_type i = 0; i < haystack.length(); i++)
		if(haystack[i] == needle)
			return i;
	return std::string::npos;
}

size_t String::lastIndexOf(char c) const
{
	if(empty())
		return std::string::npos;
	for (size_t i = size() - 1; ; i--) {
		if(at(i) == c)
			return i;
		if(i == 0)
			break;
	}
	return std::string::npos;
}

bool String::endsWith(const std::string &str, const std::string &with)
{
	if(str.size() < with.size())
		return false;
	auto ix = str.find(with, str.size() - with.size());
	return ix == (str.size() - with.size());
}

std::string String::mid(size_t pos, size_t cnt) const
{
	if(pos < size())
		return substr(pos, cnt);
	return {};
}

bool String::equal(std::string const& a, std::string const& b, String::CaseSensitivity case_sensitivity)
{
	if (a.length() == b.length()) {
		return std::equal(
					b.begin(), b.end(),
					a.begin(),
					(case_sensitivity == CaseInsensitive) ?
						[](char x, char y) { return std::tolower(x) == std::tolower(y); }:
		[](char x, char y) { return x == y;}
		);
	}
	else {
		return false;
	}
}

std::vector<std::string> String::split(const std::string &str, char delim, SplitBehavior split_behavior)
{
	using namespace std;
	vector<string> ret;
	size_t pos = 0;
	while(true) {
		size_t pos2 = str.find_first_of(delim, pos);
		//shvWarning() << pos << str << pos2;
		string s = (pos2 == string::npos)? str.substr(pos): str.substr(pos, pos2 - pos);
		if(split_behavior == KeepEmptyParts || !s.empty())
			ret.push_back(s);
		if(pos2 == string::npos)
			break;
		pos = pos2 + 1;
	}
	return ret;
}

std::string String::join(const std::vector<std::string> &lst, const std::string &delim)
{
	std::string ret;
	for(const auto &s : lst) {
		if(!ret.empty())
			ret += delim;
		ret += s;
	}
	return ret;
}

std::string String::join(const std::vector<std::string> &lst, char delim)
{
	std::string ret;
	for(const auto &s : lst) {
		if(!ret.empty())
			ret += delim;
		ret += s;
	}
	return ret;
}

std::string String::join(const std::vector<StringView> &lst, char delim)
{
	std::string ret;
	for(const auto &s : lst) {
		if(!ret.empty())
			ret += delim;
		ret += s.toString();
	}
	return ret;
}

int String::replace(std::string &str, const std::string &from, const std::string &to)
{
	int i = 0;
	size_t pos = 0;
	for (i = 0; ; ++i) {
		pos = str.find(from, pos);
		if(pos == std::string::npos)
			break;
		str.replace(pos, from.length(), to);
		pos += to.length();
	}
	return i;
}

int String::replace(std::string& str, const char from, const char to)
{
	int n = 0;
	for (char& i : str) {
		if(i == from) {
			i = to;
			n++;
		}
	}
	return n;
}

std::string &String::upper(std::string &s)
{
	for (char& i : s)
		i = static_cast<char>(std::toupper(i));
	return s;
}

std::string &String::lower(std::string &s)
{
	for (char& i : s)
		i = static_cast<char>(std::tolower(i));
	return s;
}

static size_t find_str(const std::string &haystack, size_t begin_pos, size_t end_pos, const std::string &needle)
{
	using namespace std;
	if(needle.empty())
		return string::npos;
	if(begin_pos > haystack.size())
		return string::npos;
	if(end_pos > haystack.size())
		return string::npos;
	if(end_pos < begin_pos)
		return string::npos;
	if(needle.size() > (end_pos - begin_pos))
		return string::npos;
	auto pos1 = begin_pos;
	auto pos2 = end_pos - needle.size() + 1;
	while(pos1 < pos2) {
		//std::cout << "it1: " << *it1 << '\n';
		unsigned i = 0;
		for(; i<needle.size(); ++i) {
			//std::cout << i << " it: " << *(it1 + i) << " str: " << str[i] << '\n';
			if(haystack[pos1 + i] != needle[i])
				break;
		}
		if(i == needle.size())
			return pos1;
		++pos1;
	}
	return string::npos;
}

std::pair<size_t, size_t> String::indexOfBrackets(const std::string &haystack, size_t begin_pos, size_t end_pos, const std::string &open_bracket, const std::string &close_bracket)
{
	if(begin_pos + open_bracket.size() > haystack.size())
		return std::pair<size_t, size_t>(std::string::npos, std::string::npos);
	if(end_pos > haystack.size())
		end_pos = haystack.size();
	if(end_pos < begin_pos)
		return std::pair<size_t, size_t>(std::string::npos, std::string::npos);
	//std::cout << "=====: " << string(str1, str2) << '\n';
	auto open0 = find_str(haystack, begin_pos, end_pos, open_bracket);
	auto open1 = open0;
	auto close1 = find_str(haystack, open1, end_pos, close_bracket);
	int nest_cnt = 0;
	while(true) {
		//std::cout << "open1 : " << string(open1, str2) << '\n';
		//std::cout << "close1: " << string(close1, str2) << '\n';
		if(open1 == std::string::npos || close1 == std::string::npos)
			return std::pair<size_t, size_t>(open0, close1);
		open1 = find_str(haystack, open1 + open_bracket.size(), close1, open_bracket);
		//std::cout << "\topen2 : " << string(open1, str2) << '\n';
		if(open1 == std::string::npos) {
			// no opening bracket before closing, we have found balaced pair if nest_cnt == 0
			if(nest_cnt == 0)
				return std::pair<size_t, size_t>(open0, close1);
			nest_cnt--;
			open1 = close1;
			close1 = find_str(haystack, close1 + close_bracket.size(), end_pos, close_bracket);
			//std::cout << "\tclose2 : " << string(open1, str2) << '\n';
		}
		else {
			nest_cnt++;
		}
	}
}
#ifdef TEST_BRACKETS
int main()
{
	for(const string &s : {
		"{{ahoj0}}",
		"{{ah{o}j0}}",
		"fdwre {{ahoj}} babi {{ahoj?=htu{{babi}}jede,neco1,neco2,{{neco3}}ocasek}} fr",
		"ahoj}}",
		"{{ahoj1",
		"{{{ahoj2}",
		"{{{ahoj3}}",
		"{{{{ahoj4}}}}",
	}) {
		std::cout << "---------------------------------------------" << '\n';
		std::cout << s << '\n';
		std::cout << "---------------------------------------------" << '\n';
		tuple<size_t, size_t> range(0, 0);
		while(true) {
			range = indexOfBrackets(s, get<0>(range), s.size(), "{{", "}}");
			//std::cout << "r0: " << string(get<0>(range), s.end()) <<'\n';
			//std::cout << "r1: " << string(get<1>(range), s.end()) <<'\n';
			if(std::get<0>(range) == string::npos)
				break;
			if(std::get<1>(range) == string::npos)
				break;
			auto b = get<0>(range);
			auto e = get<1>(range) + 2;
			std::string s2(s, b, e - b);
			std::cout << "hit: " << s2 <<'\n';
			get<0>(range) = e;
		}
	}
}
#endif
}
