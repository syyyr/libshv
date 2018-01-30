#include "utils.h"

#include <regex>

namespace shv {
namespace chainpack {

std::string Utils::removeJsonComments(const std::string &json_str)
{
	// http://blog.ostermiller.org/find-comment
	const std::regex re_block_comment("/\\*(?:.|[\\n])*?\\*/");
	const std::regex re_line_comment("//.*[\\n]");
	std::string result1 = std::regex_replace(json_str, re_block_comment, std::string());
	std::string ret = std::regex_replace(result1, re_line_comment, std::string());
	return ret;
}

std::string Utils::binaryDump(const std::string &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ++i) {
		uint8_t u = bytes[i];
		if(i > 0)
			ret += '|';
		for (size_t j = 0; j < 8*sizeof(u); ++j) {
			ret += (u & (((uint8_t)128) >> j))? '1': '0';
		}
	}
	return ret;
}

static inline char hex_nibble(char i)
{
	if(i < 10)
		return '0' + i;
	return 'A' + (i - 10);
}

std::string Utils::toHex(const std::string &bytes, size_t start_pos)
{
	std::string ret;
	for (size_t i = start_pos; i < bytes.size(); ++i) {
		unsigned char b = bytes[i];
		char h = b / 16;
		char l = b % 16;
		ret += hex_nibble(h);
		ret += hex_nibble(l);
	}
	return ret;
}

std::string Utils::toHex(const std::basic_string<uint8_t> &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ++i) {
		unsigned char b = bytes[i];
		char h = b / 16;
		char l = b % 16;
		ret += hex_nibble(h);
		ret += hex_nibble(l);
	}
	return ret;
}

static inline char unhex_char(char c)
{
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return char(0);
}

std::string Utils::fromHex(const std::string &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ) {
		unsigned char u = unhex_char(bytes[i++]);
		u = 16 * u;
		if(i < bytes.size())
			u += unhex_char(bytes[i++]);
		ret.push_back(u);
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
