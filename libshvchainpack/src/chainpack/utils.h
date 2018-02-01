#pragma once

#include "../shvchainpackglobal.h"

#include <limits>
#include <string>

#ifdef LIBC_NEWLIB
#include <sstream>
#endif

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT Utils
{
public:
	static std::string removeJsonComments(const std::string &json_str);

	std::string binaryDump(const std::string &bytes);
	static std::string toHexElided(const std::string &bytes, size_t start_pos, size_t max_len = 0);
	static std::string toHex(const std::string &bytes, size_t start_pos = 0, size_t length = std::numeric_limits<size_t>::max());
	static std::string toHex(const std::basic_string<uint8_t> &bytes);
	static std::string fromHex(const std::string &bytes);

	template<typename T>
	static std::string toString(T i)
	{
#ifdef LIBC_NEWLIB
		std::ostringstream ss;
		ss << i;
		return ss.str();
#else
		return std::to_string(i); //not supported by newlib
#endif
	}
};


} // namespace chainpack
} // namespace shv
