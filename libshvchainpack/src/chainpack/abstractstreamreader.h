#pragma once

#include "rpcvalue.h"

#include "../shvchainpackglobal.h"

#include <istream>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT AbstractStreamReader
{
public:
	class SHVCHAINPACK_DECL_EXPORT ParseException : public std::runtime_error
	{
		using Super = std::runtime_error;
	public:
		using Super::Super;
	};
public:
	AbstractStreamReader(std::istream &in);
	virtual ~AbstractStreamReader() {}

	RpcValue read();

	virtual void read(RpcValue::MetaData &meta_data) = 0;
	virtual void read(RpcValue &val) = 0;
protected:
	std::istream &m_in;
};

} // namespace chainpack
} // namespace shv

