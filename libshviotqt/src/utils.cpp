#include "utils.h"

#include <shv/chainpack/rpcvalue.h>

#include <QVariant>
#include <QDateTime>

namespace shv {
namespace iotqt {

QVariant Utils::rpcValueToQVariant(const chainpack::RpcValue &v)
{
	switch (v.type()) {
	case chainpack::RpcValue::Type::Invalid: return QVariant();
	case chainpack::RpcValue::Type::Null: return QVariant();
	case chainpack::RpcValue::Type::UInt: return QVariant(v.toUInt());
	case chainpack::RpcValue::Type::Int: return QVariant(v.toInt());
	case chainpack::RpcValue::Type::Double: return QVariant(v.toDouble());
	case chainpack::RpcValue::Type::Bool: return QVariant(v.toBool());
	case chainpack::RpcValue::Type::String: return QVariant(QString::fromStdString(v.toString()));
	case chainpack::RpcValue::Type::DateTime: return QDateTime::fromMSecsSinceEpoch(v.toDateTime().msecsSinceEpoch());
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
	return QString::fromStdString(v.toString());
}

chainpack::RpcValue Utils::qVariantToRpcValue(const QVariant &v)
{
	if(!v.isValid())
		return chainpack::RpcValue();
	if(v.isNull())
		return chainpack::RpcValue(nullptr);
	switch (v.type()) {
	case QVariant::UInt: return chainpack::RpcValue(v.toUInt());
	case QVariant::Int: return chainpack::RpcValue(v.toInt());
	case QVariant::Double: return chainpack::RpcValue(v.toDouble());
	case QVariant::Bool: return chainpack::RpcValue(v.toBool());
	case QVariant::String: return v.toString().toStdString();
	case QVariant::DateTime: return chainpack::RpcValue::DateTime::fromMSecsSinceEpoch(v.toDateTime().toMSecsSinceEpoch());
	case QVariant::List: {
		chainpack::RpcValue::List lst;
		for(const QVariant &rv : v.toList())
			lst.push_back(qVariantToRpcValue(rv));
		return chainpack::RpcValue{lst};
	}
	case QVariant::Map: {
		chainpack::RpcValue::Map map;
		QMapIterator<QString, QVariant> it(v.toMap());
		while (it.hasNext()) {
			it.next();
			map[it.key().toStdString()] = qVariantToRpcValue(it.value());
		}
		return chainpack::RpcValue{map};
	}
	default:
		return v.toString().toStdString();
	}
}

}}
