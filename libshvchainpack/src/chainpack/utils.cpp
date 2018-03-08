#include "utils.h"

#include <regex>
#include <iomanip>

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

static std::string byte_to_hex( uint8_t i )
{
	std::string ret;
	char h = i / 16;
	char l = i % 16;
	ret += hex_nibble(h);
	ret += hex_nibble(l);
	return ret;
}

template<typename U>
static std::string int_to_hex( U i )
{
	std::stringstream stream;
	stream << std::setfill ('0') << std::setw(sizeof(U)*2) << std::hex << i;
	return stream.str();
}

std::string Utils::toHex(const std::string &bytes, size_t start_pos, size_t length)
{
	std::string ret;
	const size_t max_pos = std::min(bytes.size(), start_pos + length);
	for (size_t i = start_pos; i < max_pos; ++i) {
		unsigned char b = bytes[i];
		ret += byte_to_hex(b);
	}
	return ret;
}

std::string Utils::toHex(const std::basic_string<uint8_t> &bytes)
{
	std::string ret;
	for (size_t i = 0; i < bytes.size(); ++i) {
		unsigned char b = bytes[i];
		ret += byte_to_hex(b);
	}
	return ret;
}

std::string Utils::toHexElided(const std::string &bytes, size_t start_pos, size_t max_len)
{
	std::string hex = toHex(bytes, start_pos, max_len + 1);
	if(hex.size() > 3 && hex.size() > max_len) {
		hex.resize(hex.size() - 1);
		for (int i = 0; i < 3; ++i)
			hex[hex.size() - 1 - i] = '.';
	}
	return hex;
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

std::string Utils::hexDump(const std::string &bytes)
{
	std::string ret;
	std::string hex_l, str_l, num_l = int_to_hex((size_t)0);
	for (size_t i = 0; i < bytes.length(); ++i) {
		auto c = bytes[i];
		std::string s = byte_to_hex(c);
		hex_l += s;
		str_l.push_back((c >= ' ' && c < 127)? c: '.');
		if(( i + 1 ) % 16 == 0) {
			ret += num_l + ' ' + hex_l + " " + str_l + '\n';
			hex_l.clear();
			str_l.clear();
			num_l = int_to_hex(i+1);
		}
		else {
			hex_l.push_back(' ');
		}
	}
	if(!hex_l.empty()) {
		static constexpr size_t hex_len = 16 * 3;
		std::string rest_l(hex_len - hex_l.length(), ' ');
		ret += num_l + ' ' + hex_l + rest_l + str_l;
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
