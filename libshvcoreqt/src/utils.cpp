#include "utils.h"

#include <shv/chainpack/rpcvalue.h>

#include <shv/core/log.h>

#include <QVariant>
#include <QDateTime>
#include <QTimeZone>

namespace shv {
namespace coreqt {

QVariant Utils::rpcValueToQVariant(const chainpack::RpcValue &v, bool *ok)
{
	if(ok)
		*ok = true;
	switch (v.type()) {
	case chainpack::RpcValue::Type::Invalid: return QVariant();
	case chainpack::RpcValue::Type::Null: return QVariant();
	case chainpack::RpcValue::Type::UInt: return QVariant(v.toUInt());
	case chainpack::RpcValue::Type::Int: return QVariant(v.toInt());
	case chainpack::RpcValue::Type::Double: return QVariant(v.toDouble());
	case chainpack::RpcValue::Type::Bool: return QVariant(v.toBool());
	case chainpack::RpcValue::Type::String: {
		const std::string &s = v.asString();
		return QVariant(QString::fromStdString(s));
	}
	case chainpack::RpcValue::Type::Blob: {
		const auto &blob = v.asBlob();
		return QVariant(QByteArray((char*)blob.data(), blob.size()));
	}
	case chainpack::RpcValue::Type::DateTime: {
		QDateTime dt = QDateTime::fromMSecsSinceEpoch(v.toDateTime().msecsSinceEpoch());
		dt.setTimeZone(QTimeZone::utc());
		return dt;
	}
	case chainpack::RpcValue::Type::List: {
		QVariantList lst;
		for(const auto &rv : v.toList())
			lst.insert(lst.size(), rpcValueToQVariant(rv));
		return lst;
	}
	case chainpack::RpcValue::Type::Map: {
		QVariantMap map;
		for(const auto &kv : v.toMap())
			map[QString::fromStdString(kv.first)] = rpcValueToQVariant(kv.second);
		return map;
	}
	case chainpack::RpcValue::Type::IMap: {
		QVariantMap map;
		for(const auto &kv : v.toIMap())
			map[QString::number(kv.first)] = rpcValueToQVariant(kv.second);
		return map;
	}
	case chainpack::RpcValue::Type::Decimal: return QVariant(v.toDouble());
	}
	if(ok) {
		*ok = false;
		return QVariant();
	}
	return QString::fromStdString(v.toString());
}

chainpack::RpcValue Utils::qVariantToRpcValue(const QVariant &v, bool *ok)
{
	if(ok)
		*ok = true;
	if(!v.isValid())
		return chainpack::RpcValue();
	//if(v.isNull()) QString() is NULL
	//	return chainpack::RpcValue(nullptr);
	switch (v.userType()) {
	case QMetaType::UChar:
	case QMetaType::UShort:
	case QMetaType::UInt: return chainpack::RpcValue(v.toUInt());
	case QMetaType::Char:
	case QMetaType::Short:
	case QMetaType::Int: return chainpack::RpcValue(v.toInt());
	case QMetaType::Float:
	case QMetaType::Double: return chainpack::RpcValue(v.toDouble());
	case QMetaType::Bool: return chainpack::RpcValue(v.toBool());
	case QMetaType::QString: return v.toString().toStdString();
	case QMetaType::QByteArray: {
		auto ba = v.toByteArray();
		auto *array = (const uint8_t*)ba.constData();
		return chainpack::RpcValue::Blob(array, array + ba.size());
	}
	case QMetaType::QDateTime: return chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(v.toDateTime().toMSecsSinceEpoch());
	case QMetaType::QStringList:
	case QMetaType::QVariantList: {
		chainpack::RpcValue::List lst;
		for(const QVariant &qv : v.toList()) {
			lst.push_back(qVariantToRpcValue(qv));
		}
		return chainpack::RpcValue{std::move(lst)};
	}
	case QMetaType::QVariantMap: {
		chainpack::RpcValue::Map map;
		QMapIterator<QString, QVariant> it(v.toMap());
		while (it.hasNext()) {
			it.next();
			map[it.key().toStdString()] = qVariantToRpcValue(it.value());
		}
		return chainpack::RpcValue{std::move(map)};
	}
	default:
		if(ok) {
			*ok = false;
			shvWarning() << "Cannot convert QVariant type:" << v.typeName() << "to RpcValue";
			return chainpack::RpcValue();
		}
		return v.toString().toStdString();
	}
}

QStringList Utils::rpcValueToStringList(const shv::chainpack::RpcValue &rpcval)
{
	QStringList ret;
	for(const chainpack::RpcValue &v : rpcval.toList())
		ret << QString::fromStdString(v.toString());
	return ret;
}

shv::chainpack::RpcValue Utils::stringListToRpcValue(const QStringList &sl)
{
	shv::chainpack::RpcValue::List ret;
	for(const QString &s : sl)
		ret.push_back(s.toStdString());
	return shv::chainpack::RpcValue(ret);
}

} // namespace coreqt
} // namespace shv
