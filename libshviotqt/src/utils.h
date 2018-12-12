#pragma once

#include "shviotqtglobal.h"

class QVariant;

namespace shv {
namespace chainpack { class RpcValue; }
namespace iotqt {

class SHVIOTQT_DECL_EXPORT Utils
{
public:
	static QVariant rpcValueToQVariant(const chainpack::RpcValue &v);
	static chainpack::RpcValue qVariantToRpcValue(const QVariant &v);
};

}}
