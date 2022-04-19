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
	static void registerQtMetaTypes();
};

} // namespace chainack
} // namespace coreqt
} // namespace shv

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
