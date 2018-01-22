#pragma once

#include "../shvchainpackglobal.h"

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
	static std::string toHex(const std::string &bytes);
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
