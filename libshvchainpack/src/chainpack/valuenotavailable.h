#ifndef SHV_CHAINPACK_VALUENOTAVAILABLE_H
#define SHV_CHAINPACK_VALUENOTAVAILABLE_H

#include "../shvchainpackglobal.h"
#include "metatypes.h"

namespace shv {
namespace chainpack {

class RpcValue;

class SHVCHAINPACK_DECL_EXPORT ValueNotAvailable
{
public:
	class MetaType : public chainpack::meta::MetaType
	{
		using Super = chainpack::meta::MetaType;
	public:
		enum {ID = chainpack::meta::GlobalNS::MetaTypeId::ValueNotAvailable};
		MetaType();
		static void registerMetaType();
	};
public:
	ValueNotAvailable() {}

	static bool isValueNotAvailable(const RpcValue &rv);
	shv::chainpack::RpcValue toRpcValue() const;
};

} // namespace chainpack
} // namespace shv

#endif // SHV_CHAINPACK_VALUENOTAVAILABLE_H
