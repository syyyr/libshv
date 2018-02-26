#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <QMetaType>

Q_DECLARE_METATYPE(shv::chainpack::RpcValue)

namespace shv {
namespace iotqt {
namespace chainpack {

class SHVIOTQT_DECL_EXPORT Rpc
{
public:
	static void registerMetatTypes();
};

} // namespace chainack
} // namespace coreqt
} // namespace shv
