#pragma once

#include "../shviotqtglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <QDateTime>
#include <QMetaType>

Q_DECLARE_METATYPE(shv::chainpack::RpcValue)

namespace shv {
namespace iotqt {
namespace rpc {

class SHVIOTQT_DECL_EXPORT Rpc
{
public:
	static void registerMetaTypes();
	static shv::chainpack::RpcValue::DateTime toRpcDateTime(const QDateTime &d);
};

} // namespace chainack
} // namespace coreqt
} // namespace shv

template<> inline QString rpcvalue_cast<QString>(const shv::chainpack::RpcValue &v) { return QString::fromStdString(v.toString()); }
template<> inline QDateTime rpcvalue_cast<QDateTime>(const shv::chainpack::RpcValue &v)
{
	if (!v.isValid() || !v.isDateTime()) {
		return QDateTime();
	}
	return QDateTime::fromMSecsSinceEpoch(v.toDateTime().msecsSinceEpoch(), Qt::TimeSpec::UTC);
}

namespace shv {
namespace chainpack {

template<> inline shv::chainpack::RpcValue RpcValue::fromValue<QDateTime>(const QDateTime &d) { return shv::iotqt::rpc::Rpc::toRpcDateTime(d); }
template<> inline shv::chainpack::RpcValue RpcValue::fromValue<QString>(const QString &s) { return s.toStdString(); }

} // namespace chainack
} // namespace shv
