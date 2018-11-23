#pragma once

#include "shvcoreglobal.h"

#include <sstream>
#include <string>
#include <vector>

namespace shv {
namespace core {

class StringView;

class SHVCORE_DECL_EXPORT String : public std::string
{
	using Super = std::string;
public:
	enum CaseSensitivity {CaseSensitive, CaseInsensitive};
	enum SplitBehavior {KeepEmptyParts, SkipEmptyParts};

	using Super::Super;
	String(const std::string &o) : Super(o) {}
	String(std::string &&o) : Super(o) {}

	static bool equal(const std::string &a, const std::string &b, String::CaseSensitivity case_sensitivity);
	static std::string::size_type indexOf(const std::string & str_haystack, const std::string &str_needle, String::CaseSensitivity case_sensitivity);
	bool startsWith(const std::string &with) const {return startsWith(*this, with);}
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

	static std::string& upper(std::string& s);
	static std::string toUpper(const std::string& s) {std::string ret(s); return upper(ret);}
	static std::string& lower(std::string& s);
	static std::string toLower(const std::string& s) {std::string ret(s); return lower(ret);}

	template<typename T>
	static std::string toString(T i, size_t width = 0, char fill_char = ' ')
	{
		std::ostringstream ss;
		ss << i;
		std::string ret = ss.str();
		while(ret.size() < width)
			ret = fill_char + ret;
		return ret;
	}
	inline static int toInt(const std::string &str, bool *ok)
	{
		int ret = 0;
		bool is_ok = false;
		try {
			size_t pos;
			ret = std::stoi(str, &pos);
			if(pos == str.length())
				is_ok = true;
			else
				ret = 0;
		}
		catch (...) {
			ret = 0;
		}
		if(ok)
			*ok = is_ok;
		return ret;
	}
	inline static double toDouble(const std::string &str, bool *ok)
	{
		double ret = 0;
		bool is_ok = false;
		try {
			size_t pos;
			ret = std::stod(str, &pos);
			if(pos == str.length())
				is_ok = true;
			else
				ret = 0;
		}
		catch (...) {
			ret = 0;
		}
		if(ok)
			*ok = is_ok;
		return ret;
	}
};

}}
