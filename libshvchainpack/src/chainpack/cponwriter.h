#pragma once

#include "abstractstreamwriter.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT CponWriterOptions
{
	bool m_translateIds = false;
	bool m_indent = 0;
public:
	//CponWriterOptions() {}

	bool setTranslateIds() const {return m_translateIds;}
	CponWriterOptions& setTranslateIds(bool b) {m_translateIds = b; return *this;}

	bool isIndent() const {return m_indent;}
	CponWriterOptions& setIndent(bool i) {m_indent = i; return *this;}
};

class SHVCHAINPACK_DECL_EXPORT CponWriter : public AbstractStreamWriter
{
	using Super = AbstractStreamWriter;

public:
	CponWriter(std::ostream &out) : Super(out) {}
	CponWriter(std::ostream &out, const CponWriterOptions &opts) : CponWriter(out) {m_opts = opts;}

	CponWriter& operator <<(const RpcValue &value) {write(value); return *this;}
	CponWriter& operator <<(const RpcValue::MetaData &meta_data) {write(meta_data); return *this;}

	void write(const RpcValue &val) override;
	void write(const RpcValue::MetaData &meta_data) override;

	void writeContainerBegin(RpcValue::Type container_type) override;
	void writeContainerEnd(RpcValue::Type container_type) override;
	void writeListElement(const RpcValue &val) override;
	void writeMapElement(const std::string &key, const RpcValue &val) override;
	void writeMapElement(RpcValue::UInt key, const RpcValue &val) override;
	void writeArrayBegin(RpcValue::Type, size_t) override;
	void writeArrayElement(const RpcValue &val) override {writeListElement(val);}
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
	void separateElement();
private:
	CponWriterOptions m_opts;
	int m_currentIndent = 0;
};

} // namespace chainpack
} // namespace shv

