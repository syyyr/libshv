#include "string.h"
#include "log.h"
#include "stringview.h"

#include <algorithm>

namespace shv {
namespace core {

const char* String::WhiteSpaceChars = " \t\n\r\f\v";

std::string::size_type String::indexOf(const std::string & str_haystack, const std::string &str_needle, String::CaseSensitivity case_sensitivity)
{
	auto it = std::search(
				  str_haystack.begin(), str_haystack.end(),
				  str_needle.begin(), str_needle.end(),
				  (case_sensitivity == CaseInsensitive) ?
					  [](char a, char b) { return std::tolower(a) == std::tolower(b); }:
	[](char a, char b) { return a == b;}
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

bool String::equal(std::string const& a, std::string const& b, String::CaseSensitivity case_sensitivity)
{
	if (a.length() == b.length()) {
		return std::equal(
					b.begin(), b.end(),
					a.begin(),
					(case_sensitivity == CaseInsensitive) ?
						[](char a, char b) { return std::tolower(a) == std::tolower(b); }:
		[](char a, char b) { return a == b;}
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
	for (size_t i = 0; i < str.length(); ++i) {
		if(str[i] == from) {
			str[i] = to;
			n++;
		}
	}
	return n;
}

std::string &String::upper(std::string &s)
{
	for (size_t i = 0; i < s.size(); ++i)
		s[i] = std::toupper(s[i]);
	return s;
}

std::string &String::lower(std::string &s)
{
	for (size_t i = 0; i < s.size(); ++i)
		s[i] = std::tolower(s[i]);
	return s;
}

}}
