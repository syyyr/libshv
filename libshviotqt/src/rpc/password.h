#ifndef SHV_IOTQT_RPC_PASSWORD_H
#define SHV_IOTQT_RPC_PASSWORD_H

#include "../shviotqtglobal.h"

#include <string>

namespace shv {
namespace iotqt {
namespace rpc {

struct SHVIOTQT_DECL_EXPORT Password
{
public:
	enum class Format {Invalid, Plain, Sha1};

	std::string password;
	Format format = Format::Invalid;

	static const char *formatToString(Format f);
	static Format formatFromString(const std::string &s);
};

} // namespace rpc
} // namespace iotqt
} // namespace shv

#endif // SHV_IOTQT_RPC_PASSWORD_H
