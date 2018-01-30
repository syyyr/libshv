#pragma once

#include "../shvchainpackglobal.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT Rpc
{
public:
	enum class ProtocolVersion {Invalid = 0, ChainPack, Cpon, /*Json*/};
	static const char* METH_HELLO;
	static const char* METH_LOGIN;
};

} // namespace chainpack
} // namespace shv
