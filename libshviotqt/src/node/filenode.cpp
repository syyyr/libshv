#include "filenode.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>
#include <shv/coreqt/log.h>

#include <QCryptographicHash>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace node {

static const char *M_HASH = "hash";
static const char *M_SIZE = "size";
static const char *M_SIZE_COMPRESSED = "sizeCompressed";
static const char *M_READ = "read";
static const char *M_READ_COMPRESSED = "readCompressed";

const std::vector<shv::chainpack::MetaMethod> FileNode::meta_methods_file_base = {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE},
	{M_HASH, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ},
	{M_SIZE, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::LargeResultHint, cp::Rpc::ROLE_BROWSE},
	{M_SIZE_COMPRESSED, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_BROWSE, "Parameters\n - compressionType: gzip (default) | qcompress"},
	{M_READ, cp::MetaMethod::Signature::RetVoid, cp::MetaMethod::Flag::LargeResultHint, cp::Rpc::ROLE_READ},
	{M_READ_COMPRESSED, cp::MetaMethod::Signature::RetParam, cp::MetaMethod::Flag::None, cp::Rpc::ROLE_READ, "Parameters\n - compressionType: gzip (default) | qcompress"},
};

enum class CompressionType {
	Invalid,
	GZip,
	QCompress,
};

static CompressionType compression_type_from_string(const std::string &type_str, CompressionType default_type)
{
	if (type_str.empty())
		return default_type;
	else if (type_str == "gzip")
		return CompressionType::GZip;
	else if (type_str == "qcompress")
		return CompressionType::QCompress;
	else
		return CompressionType::Invalid;
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

FileNode::FileNode(const std::string &node_id, shv::iotqt::node::FileNode::Super *parent)
	: Super(node_id, parent)
{
}

cp::RpcValue FileNode::callMethod(const shv::iotqt::node::ShvNode::StringViewList &shv_path, const std::string &method, const shv::chainpack::RpcValue &params, const shv::chainpack::RpcValue &user_id)
{
	if (method == M_READ) {
		return read(shv_path);
	}
	if (method == M_READ_COMPRESSED) {
		return readFileCompressed(shv_path, params);
	}
	if(method == M_HASH) {
		shv::chainpack::RpcValue::Blob bytes = read(shv_path).asBlob();
		QCryptographicHash h(QCryptographicHash::Sha1);
		h.addData((const char*)bytes.data(), bytes.size());
		return h.result().toHex().toStdString();
	}
	if(method == M_SIZE) {
		return size(shv_path);
	}
	if(method == M_SIZE_COMPRESSED) {
		return readFileCompressed(shv_path, params).asBlob().size();
	}

	return Super::callMethod(shv_path, method, params, user_id);
}

size_t FileNode::methodCount(const ShvNode::StringViewList &shv_path)
{
	Q_UNUSED(shv_path)

	return meta_methods_file_base.size();
}

const chainpack::MetaMethod *FileNode::metaMethod(const ShvNode::StringViewList &shv_path, size_t ix)
{
	Q_UNUSED(shv_path)

	if(meta_methods_file_base.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_file_base.size()));
	return &meta_methods_file_base[ix];
}

std::string FileNode::fileName(const ShvNode::StringViewList &shv_path) const
{
	Q_UNUSED(shv_path)
	return nodeId();
}

chainpack::RpcValue FileNode::read(const ShvNode::StringViewList &shv_path) const
{
	cp::RpcValue ret_value = readContent(shv_path);
	ret_value.setMetaValue("fileName", fileName(shv_path));
	return ret_value;
}

chainpack::RpcValue FileNode::size(const ShvNode::StringViewList &shv_path) const
{
	return read(shv_path).asBlob().size();
}

chainpack::RpcValue FileNode::readFileCompressed(const ShvNode::StringViewList &shv_path, const chainpack::RpcValue &params) const
{
	const auto compression_type_str = params.asMap().value("compressionType").toString();
	const auto compression_type = compression_type_from_string(compression_type_str, CompressionType::GZip);
	if (compression_type == CompressionType::Invalid) {
		SHV_EXCEPTION("Invalid compression type: " + compression_type_str);
	}

	cp::RpcValue result;
	const cp::RpcValue::Blob blob = readContent(shv_path).asBlob();
	if (compression_type == CompressionType::QCompress) {
		const auto compressed_blob = qCompress(QByteArray::fromRawData(reinterpret_cast<const char *>(blob.data()), blob.size()));
		result = shv::chainpack::RpcValue::Blob(compressed_blob.cbegin(), compressed_blob.cend());

		result.setMetaValue("compressionType", "qcompress");
		result.setMetaValue("fileName", fileName(shv_path) + ".qcompress");
	}
	else if (compression_type == CompressionType::GZip) {
		QByteArray compressed_blob = qCompress(QByteArray::fromRawData(reinterpret_cast<const char *>(blob.data()), blob.size()));

		// Remove 4 bytes of length added by Qt a 2 bytes of zlib header from the beginning
		compressed_blob = compressed_blob.mid(6);

		// Remove 4 bytes of ADLER-32 zlib checksum from the end
		compressed_blob.chop(4);

		// GZIP header according to GZIP File Format Specification (RFC 1952)
		static const char gzip_header[] = {'\x1f', '\x8b', '\x08', '\x00', '\x00', '\x00', '\x00', '\x00', '\x00', '\x03'};
		compressed_blob.prepend(gzip_header, sizeof(gzip_header));

		const uint32_t crc32 = crc32_checksum(blob.data(), blob.size());
		compressed_blob.append(crc32 & 0xff);
		compressed_blob.append((crc32 >> 8) & 0xff);
		compressed_blob.append((crc32 >> 16) & 0xff);
		compressed_blob.append((crc32 >> 24) & 0xff);

		const uint32_t data_size = blob.size();
		compressed_blob.append(data_size & 0xff);
		compressed_blob.append((data_size >> 8) & 0xff);
		compressed_blob.append((data_size >> 16) & 0xff);
		compressed_blob.append((data_size >> 24) & 0xff);

		result = shv::chainpack::RpcValue::Blob(compressed_blob.cbegin(), compressed_blob.cend());

		result.setMetaValue("compressionType", "gzip");
		result.setMetaValue("fileName", fileName(shv_path) + ".gz");
	}

	return result;
}

}}}
