#pragma once

#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT AbstractStreamWriter
{
public:
	class ListElement : public RpcValue
	{
		friend class CponWriter;
		const RpcValue &val;
	public:
		ListElement(const RpcValue &v) : val(v) {}
	};
	class MapElement
	{
		friend class CponWriter;
		const std::string &key;
		const RpcValue &val;
	public:
		MapElement(const std::string &k, const RpcValue &v) : key(k), val(v) {}
	};
	class IMapElement
	{
		friend class CponWriter;
		const RpcValue::UInt key;
		const RpcValue &val;
	public:
		IMapElement(RpcValue::UInt k, const RpcValue &v) : key(k), val(v) {}
	};
public:
	AbstractStreamWriter();
};

} // namespace chainpack
} // namespace shv

