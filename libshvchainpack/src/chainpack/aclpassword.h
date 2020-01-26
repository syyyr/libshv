#ifndef SHV_CHAINPACK_ACLPASSWORD_H
#define SHV_CHAINPACK_ACLPASSWORD_H

#include "../shvchainpackglobal.h"

#include <string>

namespace shv {
namespace chainpack {

class RpcValue;

struct SHVCHAINPACK_DECL_EXPORT AclPassword
{
	enum class Format {Invalid, Plain, Sha1};

	std::string password;
	Format format = Format::Invalid;

	AclPassword() {}
	AclPassword(std::string password, Format format)
		: password(std::move(password))
		, format(format)
	{}

	bool isValid() const {return format != Format::Invalid;}

	RpcValue toRpcValueMap() const;
	static AclPassword fromRpcValue(const RpcValue &v);

	static const char *formatToString(Format f);
	static Format formatFromString(const std::string &s);
};

} // namespace chainpack
} // namespace shv

#endif // SHV_CHAINPACK_ACLPASSWORD_H
