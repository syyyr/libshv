#pragma once

#include "abstractstreamwriter.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT CponWriterOptions
{
	bool m_translateIds = false;
	std::string m_indent;
public:
	//CponWriterOptions() {}

	bool isTranslateIds() const {return m_translateIds;}
	CponWriterOptions& setTranslateIds(bool b) {m_translateIds = b; return *this;}

	const std::string& indent() const {return m_indent;}
	CponWriterOptions& setIndent(const std::string& i) {m_indent = i; return *this;}
};

class SHVCHAINPACK_DECL_EXPORT CponWriter : public AbstractStreamWriter
{
	using Super = AbstractStreamWriter;

public:
	CponWriter(std::ostream &out) : Super(out) {}
	CponWriter(std::ostream &out, const CponWriterOptions &opts) : CponWriter(out) {m_opts = opts;}

	CponWriter& operator <<(const RpcValue &value) {write(value); return *this;}
	CponWriter& operator <<(const RpcValue::MetaData &meta_data) {write(meta_data); return *this;}

	size_t write(const RpcValue &val) override;
	size_t write(const RpcValue::MetaData &meta_data) override;

	void writeIMapKey(RpcValue::UInt key) override {write(key);}
	void writeContainerBegin(RpcValue::Type container_type) override;
	void writeContainerEnd(RpcValue::Type container_type) override;
	void writeArrayBegin(RpcValue::Type, size_t) override;

	void writeListElement(const RpcValue &val) override {writeListElement(val, false);}
	void writeMapElement(const std::string &key, const RpcValue &val) override {writeMapElement(key, val, false);}
	void writeMapElement(RpcValue::UInt key, const RpcValue &val) override {writeMapElement(key, val, false);}
	void writeArrayElement(const RpcValue &val) override {writeListElement(val);}

	// terminating separator id OK in Cpon, but world is prettier without it
	void writeListElement(const RpcValue &val, bool without_separator);
	void writeMapElement(const std::string &key, const RpcValue &val, bool without_separator);
	void writeMapElement(RpcValue::UInt key, const RpcValue &val, bool without_separator);
	void writeArrayElement(const RpcValue &val, bool without_separator) {writeListElement(val, without_separator);}
private:
	CponWriter& write(std::nullptr_t);
	CponWriter& write(bool value);
	CponWriter& write(RpcValue::Int value);
	CponWriter& write(RpcValue::UInt value);
	CponWriter& write(double value);
	CponWriter& write(RpcValue::Decimal value);
	CponWriter& write(RpcValue::DateTime value);
	CponWriter& write(const std::string &value);
	CponWriter& write(const RpcValue::Blob &value);
	CponWriter& write(const RpcValue::List &values);
	CponWriter& write(const RpcValue::Array &values);
	CponWriter& write(const RpcValue::Map &values);
	CponWriter& write(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data = nullptr);
private:
	void writeIMapContent(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data = nullptr);
	void writeMapContent(const RpcValue::Map &values);

	void startBlock();
	void endBlock();
	void indentElement();
	void separateElement(bool without_comma);
private:
	CponWriterOptions m_opts;
	int m_currentIndent = 0;
};

} // namespace chainpack
} // namespace shv

