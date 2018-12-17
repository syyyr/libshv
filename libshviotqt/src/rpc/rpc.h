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
	static void registerMetatTypes();
};

} // namespace chainack
} // namespace coreqt
} // namespace shv

template<> inline QString rpcvalue_cast<QString>(const shv::chainpack::RpcValue &v) { return QString::fromStdString(v.toString()); }
template<> inline QDateTime rpcvalue_cast<QDateTime>(const shv::chainpack::RpcValue &v)
{
	if (!v.isDateTime() || !v.toDateTime().isValid()) {
		return QDateTime();
	}
	return QDateTime::fromMSecsSinceEpoch(v.toDateTime().msecsSinceEpoch(), Qt::TimeSpec::UTC);
}

SHVIOTQT_DECL_EXPORT shv::chainpack::RpcValue::DateTime toRpcDateTime(const QDateTime &d);

template<class T> shv::chainpack::RpcValue toRpcValue(const T &);
template<> inline shv::chainpack::RpcValue toRpcValue<QDateTime>(const QDateTime &d) {	return toRpcDateTime(d); }
template<> inline shv::chainpack::RpcValue toRpcValue<QString>(const QString &s) {	return s.toStdString(); }
