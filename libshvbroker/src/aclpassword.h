#pragma once

#include "shvbrokerglobal.h"

#include <string>

namespace shv { namespace chainpack { class RpcValue; } }

namespace shv {
namespace broker {

struct SHVBROKER_DECL_EXPORT AclPassword
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

	shv::chainpack::RpcValue toRpcValueMap() const;
	static AclPassword fromRpcValue(const shv::chainpack::RpcValue &v);

	static const char *formatToString(Format f);
	static Format formatFromString(const std::string &s);
};

} // namespace chainpack
} // namespace shv

