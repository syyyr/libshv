#pragma once

#include "shvcoreqtglobal.h"

#include <shv/chainpack/rpcvalue.h>

#include <QDateTime>
#include <QMetaType>

namespace shv {
namespace coreqt {
namespace rpc {

SHVCOREQT_DECL_EXPORT void registerQtMetaTypes();
SHVCOREQT_DECL_EXPORT QVariant rpcValueToQVariant(const chainpack::RpcValue &v, bool *ok = nullptr);
SHVCOREQT_DECL_EXPORT chainpack::RpcValue qVariantToRpcValue(const QVariant &v, bool *ok = nullptr);
SHVCOREQT_DECL_EXPORT QStringList rpcValueToStringList(const shv::chainpack::RpcValue &rpcval);
SHVCOREQT_DECL_EXPORT shv::chainpack::RpcValue stringListToRpcValue(const QStringList &sl);

}}}

template<> inline QString rpcvalue_cast<QString>(const shv::chainpack::RpcValue &v) { return QString::fromStdString(v.asString()); }
template<> inline QDateTime rpcvalue_cast<QDateTime>(const shv::chainpack::RpcValue &v)
{
	if (!v.isValid() || !v.isDateTime()) {
		return QDateTime();
	}
	return QDateTime::fromMSecsSinceEpoch(v.toDateTime().msecsSinceEpoch(), Qt::TimeSpec::UTC);
}

namespace shv {
namespace chainpack {

template<> inline shv::chainpack::RpcValue RpcValue::fromValue<QString>(const QString &s) { return s.toStdString(); }
template<> inline shv::chainpack::RpcValue RpcValue::fromValue<QDateTime>(const QDateTime &d)
{
	if (d.isValid()) {
		shv::chainpack::RpcValue::DateTime dt = shv::chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(d.toUTC().toMSecsSinceEpoch());
		int offset = d.offsetFromUtc();
		dt.setTimeZone(offset / 60);
		return dt;
	}
	else {
		return shv::chainpack::RpcValue();
	}
}

} // namespace chainack
} // namespace shv

Q_DECLARE_METATYPE(shv::chainpack::RpcValue)
