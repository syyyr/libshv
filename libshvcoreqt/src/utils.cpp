#include "utils.h"
#include "rpc.h"

#include <shv/chainpack/rpcvalue.h>

#include <shv/core/log.h>

#include <QStringView>
#include <QVariant>
#include <QDateTime>

namespace shv::coreqt {

QVariant Utils::rpcValueToQVariant(const chainpack::RpcValue &v, bool *ok)
{
	return rpc::rpcValueToQVariant(v, ok);
}

chainpack::RpcValue Utils::qVariantToRpcValue(const QVariant &v, bool *ok)
{
	return rpc::qVariantToRpcValue(v, ok);
}

QStringList Utils::rpcValueToStringList(const chainpack::RpcValue &rpcval)
{
	return rpc::rpcValueToStringList(rpcval);
}

chainpack::RpcValue Utils::stringListToRpcValue(const QStringList &sl)
{
	return rpc::stringListToRpcValue(sl);
}

std::string Utils::joinPath(const std::string& p1, const std::string& p2)
{
	return core::Utils::joinPath(core::StringView(p1), core::StringView(p2));
}

QString Utils::joinPath(const QString &p1, const QString &p2)
{
	QStringView sv1(p1);
	while(!sv1.isEmpty() && sv1.last() == '/')
		sv1.chop(1);
	QStringView sv2(p2);
	while(!sv2.isEmpty() && sv2.first() == '/')
		sv2 = sv2.mid(1);
	if(sv2.isEmpty())
		return sv1.toString();
	if(sv1.isEmpty())
		return sv2.toString();
	return sv1.toString() + '/' + sv2.toString();
}

bool Utils::isValueNotAvailable(const QVariant &val)
{
	return !val.isValid();
}

bool Utils::isDefaultQVariantValue(const QVariant &val)
{
	if(!val.isValid())
		return true;
	if(val.isNull())
		return true;
	switch(val.userType()) {
	case QMetaType::LongLong:
	case QMetaType::ULongLong:
	case QMetaType::Short:
	case QMetaType::Long:
	case QMetaType::ULong:
	case QMetaType::Int:
	case QMetaType::UInt:
	case QMetaType::Char: return val.toInt() == 0;
	case QMetaType::QString: return val.toString().isEmpty();
	case QMetaType::QDateTime: return !val.toDateTime().isValid();
	default:
		return false;
	}
}

static uint32_t crc32_checksum(const uint8_t *data, int size)
{
	uint32_t crc = 0xFFFFFFFF;

	for (int i = 0; i < size; i++) {
		uint8_t byte = data[i];
		for (int j = 0; j < 8; j++) {
			uint32_t bit = (byte ^ crc) & 1;
			crc >>= 1;
			if (bit)
				crc = crc ^ 0xEDB88320;
			byte >>= 1;
		}
	}

	return ~crc;
}

std::vector<uint8_t> Utils::compressGZip(const std::vector<uint8_t> &data)
{
	QByteArray compressed_data = qCompress(QByteArray::fromRawData(reinterpret_cast<const char *>(data.data()), static_cast<int>(data.size())));

	// Remove 4 bytes of length added by Qt a 2 bytes of zlib header from the beginning
	compressed_data = compressed_data.mid(6);

	// Remove 4 bytes of ADLER-32 zlib checksum from the end
	compressed_data.chop(4);

	// GZIP header according to GZIP File Format Specification (RFC 1952)
	static const std::array gzip_header = {'\x1f', '\x8b', '\x08', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03'};
	compressed_data.prepend(gzip_header.data(), gzip_header.size());

	const uint32_t crc32 = crc32_checksum(data.data(), static_cast<int>(data.size()));
	compressed_data.append(static_cast<char>(crc32 & 0xff));
	compressed_data.append(static_cast<char>((crc32 >> 8) & 0xff));
	compressed_data.append(static_cast<char>((crc32 >> 16) & 0xff));
	compressed_data.append(static_cast<char>((crc32 >> 24) & 0xff));

	const auto data_size = static_cast<uint32_t>(data.size());
	compressed_data.append(static_cast<char>(data_size & 0xff));
	compressed_data.append(static_cast<char>((data_size >> 8) & 0xff));
	compressed_data.append(static_cast<char>((data_size >> 16) & 0xff));
	compressed_data.append(static_cast<char>((data_size >> 24) & 0xff));

	return shv::chainpack::RpcValue::Blob(compressed_data.cbegin(), compressed_data.cend());
}



} // namespace shv
