#include "utils.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/coreqt/utils.h>

#include <QCryptographicHash>
#include <QVariant>

namespace shv::iotqt::utils {
/*
QVariant Utils::rpcValueToQVariant(const shv::chainpack::RpcValue &v)
{
	return shv::coreqt::Utils::rpcValueToQVariant(v);
}

shv::chainpack::RpcValue Utils::qVariantToRpcValue(const QVariant &v)
{
	return  shv::coreqt::Utils::qVariantToRpcValue(v);
}
*/

std::string sha1Hex(const std::string &s)
{
	QCryptographicHash hash(QCryptographicHash::Algorithm::Sha1);
#if QT_VERSION_MAJOR >= 6
	hash.addData(QByteArrayView(s.data(), static_cast<int>(s.length())));
#else
	hash.addData(s.data(), static_cast<int>(s.length()));
#endif

	return std::string(hash.result().toHex().constData());
}
}
