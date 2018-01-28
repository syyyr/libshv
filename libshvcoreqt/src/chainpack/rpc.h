#pragma once

#include "../shvcoreqtglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <QMetaType>

Q_DECLARE_METATYPE(shv::chainpack::RpcValue)

namespace shv {
namespace coreqt {
namespace chainpack {

class SHVCOREQT_DECL_EXPORT Rpc
{
public:
	static void registerMetatTypes();
};

} // namespace chainack
} // namespace coreqt
} // namespace shv
