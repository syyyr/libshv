#pragma once

#include "../shvchainpackglobal.h"

#include <limits>
#include <string>

#ifdef LIBC_NEWLIB
#include <sstream>
#endif

namespace shv {
namespace chainpack1 {

class SHVCHAINPACK_DECL_EXPORT Utils
{
public:
	static std::string removeJsonComments(const std::string &json_str);

	std::string binaryDump(const std::string &bytes);
	static std::string toHexElided(const std::string &bytes, size_t start_pos, size_t max_len = 0);
	static std::string toHex(const std::string &bytes, size_t start_pos = 0, size_t length = std::numeric_limits<size_t>::max());
	static std::string toHex(const std::basic_string<uint8_t> &bytes);
	static char fromHex(char c);
	static std::string fromHex(const std::string &bytes);
	static std::string hexDump(const std::string &bytes);

	template<typename T>
	static std::string toString(T i, size_t prepend_0s_to_len = 0)
	{
#ifdef LIBC_NEWLIB
		std::ostringstream ss;
		ss << i;
		std::string ret = ss.str();
#else
		std::string ret = std::to_string(i); //not supported by newlib
#endif
		while(ret.size() < prepend_0s_to_len)
			ret = '0' + ret;
		return ret;
	}
};


} // namespace chainpack
} // namespace shv
