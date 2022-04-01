#include "valuenotavailable.h"
#include "rpcvalue.h"

using namespace std;

namespace shv {
namespace chainpack {

ValueNotAvailable::MetaType::MetaType()
	: Super("ValueNotAvailable")
{
}

void ValueNotAvailable::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		shv::chainpack::meta::registerType(shv::chainpack::meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

bool ValueNotAvailable::isValueNotAvailable(const RpcValue &rv)
{
	return rv.isValueNotAvailable();
}

RpcValue ValueNotAvailable::toRpcValue() const
{
	static RpcValue NA;
	if(!NA.isValid()) {
		NA = RpcValue{nullptr};
		NA.setMetaTypeId(meta::GlobalNS::MetaTypeId::ValueNotAvailable);
	}
	return NA;
}

} // namespace chainpack
} // namespace shv
