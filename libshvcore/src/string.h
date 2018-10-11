#pragma once

#include "shvcoreglobal.h"

#include <string>
#include <vector>

namespace shv {
namespace core {

class StringView;

class SHVCORE_DECL_EXPORT String
{
public:
	enum CaseSensitivity {CaseSensitive, CaseInsensitive};
	enum SplitBehavior {KeepEmptyParts, SkipEmptyParts};

	static bool equal(const std::string &a, const std::string &b, String::CaseSensitivity case_sensitivity);
	static std::string::size_type indexOf(const std::string & str_haystack, const std::string &str_needle, String::CaseSensitivity case_sensitivity);
	static bool startsWith(const std::string & str, const std::string &with) {return str.rfind(with, 0) == 0;}
	static bool endsWith(const std::string & str, const std::string &with) {return str.find(with, str.size() - with.size()) == (str.size() - with.size());}
	static const char * WhiteSpaceChars;
	static std::string& rtrim(std::string& s, const char* t = WhiteSpaceChars)
	{
		s.erase(s.find_last_not_of(t) + 1);
		return s;
	}
	static std::string& ltrim(std::string& s, const char* t = WhiteSpaceChars)
	{
		s.erase(0, s.find_first_not_of(t));
		return s;
	}
	static std::string& trim(std::string& s, const char* t = WhiteSpaceChars)
	{
		return ltrim(rtrim(s, t), t);
	}
	static std::vector<std::string> split(const std::string &str, char delim, SplitBehavior split_behavior = SkipEmptyParts);
	static std::string join(const std::vector<std::string> &lst, const std::string &delim);
	static std::string join(const std::vector<std::string> &lst, char delim);
	static std::string join(const std::vector<shv::core::StringView> &lst, char delim);
	static int replace(std::string &str, const std::string &from, const std::string &to);
};

}}
