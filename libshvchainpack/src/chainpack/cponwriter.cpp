#include "cponwriter.h"
//#include "chainpackprotocol.h"
#include "cpon.h"
#include "exception.h"

#include <iostream>
#include <cmath>

namespace shv {
namespace chainpack {

static constexpr bool WRITE_INVALID_AS_NULL = true;

void CponWriter::startBlock()
{
	if(m_opts.isIndent()) {
		m_out << '\n';
		m_currentIndent++;
	}
}

void CponWriter::endBlock()
{
	if(m_opts.isIndent()) {
		m_out << '\n';
		m_currentIndent--;
		indentElement();
	}
}

void CponWriter::indentElement()
{
	if(m_opts.isIndent()) {
		m_out << std::string(m_currentIndent, '\t');
	}
}

void CponWriter::separateElement()
{
	m_out << ',';
	m_out << (m_opts.isIndent()? '\n': ' ');
}

void CponWriter::write(const RpcValue &value)
{
	if(!value.metaData().isEmpty()) {
		write(value.metaData());
		if(m_opts.isIndent())
			m_out << '\n';
	}
	switch (value.type()) {
	case RpcValue::Type::Null: write(nullptr); return;
	case RpcValue::Type::UInt: write(value.toUInt()); return;
	case RpcValue::Type::Int: write(value.toInt()); return;
	case RpcValue::Type::Double: write(value.toDouble()); return;
	case RpcValue::Type::Bool: write(value.toBool()); return;
	case RpcValue::Type::Blob: write(value.toBlob()); return;
	case RpcValue::Type::String: write(value.toString()); return;
	case RpcValue::Type::DateTime: write(value.toDateTime()); return;
	case RpcValue::Type::List: write(value.toList()); return;
	case RpcValue::Type::Array: write(value.toArray()); return;
	case RpcValue::Type::Map: write(value.toMap()); return;
	case RpcValue::Type::IMap: write(value.toIMap(), &value.metaData()); return;
	case RpcValue::Type::Decimal: write(value.toDecimal()); return;
	case RpcValue::Type::Invalid:
		if(WRITE_INVALID_AS_NULL) {
			write(nullptr);
			return;
		}
	}
	SHVCHP_EXCEPTION(std::string("Don't know how to serialize type: ") + RpcValue::typeToName(value.type()));
}

void CponWriter::write(const RpcValue::MetaData &meta_data)
{
	if(!meta_data.isEmpty()) {
		m_out << Cpon::C_META_BEGIN;
		startBlock();
		const RpcValue::IMap &cim = meta_data.iValues();
		if(!cim.empty())
			writeIMapContent(cim);
		const RpcValue::Map &csm = meta_data.sValues();
		if(!csm.empty()) {
			if(!cim.empty())
				separateElement();
			writeMapContent(csm);
		}
		endBlock();
		m_out << Cpon::C_META_END;
	}
	return;
}

void CponWriter::writeContainerBegin(RpcValue::Type container_type)
{
	switch (container_type) {
	case RpcValue::Type::List:
		m_out << Cpon::C_LIST_BEGIN;
		startBlock();
		break;
	case RpcValue::Type::Map:
		m_out << Cpon::C_MAP_BEGIN;
		startBlock();
		break;
	case RpcValue::Type::IMap:
		m_out << Cpon::STR_IMAP_BEGIN;
		startBlock();
		break;
	default:
		SHVCHP_EXCEPTION(std::string("Cannot write begin of container type: ") + RpcValue::typeToName(container_type));
		break;
	}
}

void CponWriter::writeArrayBegin(RpcValue::Type , size_t )
{
	m_out << Cpon::STR_ARRAY_BEGIN;
	startBlock();
}

void CponWriter::writeContainerEnd(RpcValue::Type container_type)
{
	switch (container_type) {
	case RpcValue::Type::List:
	case RpcValue::Type::Array:
		endBlock();
		m_out << Cpon::C_LIST_END;
		startBlock();
		break;
	case RpcValue::Type::Map:
	case RpcValue::Type::IMap:
		endBlock();
		m_out << Cpon::C_MAP_END;
		break;
	default:
		SHVCHP_EXCEPTION(std::string("Cannot write end of container type: ") + RpcValue::typeToName(container_type));
		break;
	}
}

void CponWriter::writeListElement(const RpcValue &val)
{
	write(val);
	separateElement();
}

void CponWriter::writeMapElement(const std::string &key, const RpcValue &val)
{
	write(key);
	m_out << ':';
	write(val);
	separateElement();
}

void CponWriter::writeMapElement(RpcValue::UInt key, const RpcValue &val)
{
	write(key);
	m_out << ':';
	write(val);
	separateElement();
}

CponWriter &CponWriter::write(std::nullptr_t)
{
	m_out << Cpon::STR_NULL;
	return *this;
}

CponWriter &CponWriter::write(bool value)
{
	m_out << (value? Cpon::STR_TRUE : Cpon::STR_FALSE);
	return *this;
}

CponWriter &CponWriter::write(RpcValue::Int value)
{
	m_out << Utils::toString(value);
	return *this;
}

CponWriter &CponWriter::write(RpcValue::UInt value)
{
	m_out << Utils::toString(value);
	m_out << Cpon::C_UNSIGNED_END;
	return *this;
}

CponWriter &CponWriter::write(double value)
{
	if (std::isfinite(value)) {
		m_out << value;
	}
	else {
		m_out << Cpon::STR_NULL;
	}
	return *this;
}

CponWriter &CponWriter::write(RpcValue::Decimal value)
{
	m_out << value.toString() << Cpon::C_DECIMAL_END;
	return *this;
}

CponWriter &CponWriter::write(RpcValue::DateTime value)
{
	m_out << Cpon::STR_DATETIME_BEGIN << value.toUtcString() << '"';
	return *this;
}

CponWriter &CponWriter::write(const std::string &value)
{
	m_out << '"';
	for (size_t i = 0; i < value.length(); i++) {
		const char ch = value[i];
		if (ch == '\\') {
			m_out << "\\\\";
		}
		else if (ch == '"') {
			m_out << "\\\"";
		}
		else if (ch == '\b') {
			m_out << "\\b";
		}
		else if (ch == '\f') {
			m_out << "\\f";
		}
		else if (ch == '\n') {
			m_out << "\\n";
		}
		else if (ch == '\r') {
			m_out << "\\r";
		}
		else if (ch == '\t') {
			m_out << "\\t";
		}
		else if (static_cast<uint8_t>(ch) <= 0x1f) {
			char buf[8];
			snprintf(buf, sizeof buf, "\\u%04x", ch);
			m_out << buf;
		}
		else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
				 && static_cast<uint8_t>(value[i+2]) == 0xa8) {
			m_out << "\\u2028";
			i += 2;
		}
		else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
				 && static_cast<uint8_t>(value[i+2]) == 0xa9) {
			m_out << "\\u2029";
			i += 2;
		}
		else {
			m_out << ch;
		}
	}
	m_out << '"';
	return *this;
}

CponWriter &CponWriter::write(const RpcValue::Blob &value)
{
	m_out << Cpon::STR_BLOB_BEGIN;
	m_out << Utils::toHex(value);
	m_out << '"';
	return *this;
}

CponWriter &CponWriter::write(const RpcValue::List &values)
{
	writeContainerBegin(RpcValue::Type::List);
	for (size_t i = 0; i < values.size(); ) {
		indentElement();
		const RpcValue &value = values[i];
		write(value);
		if (++i < values.size())
			separateElement();
	}
	writeContainerEnd(RpcValue::Type::List);
	return *this;
}

CponWriter &CponWriter::write(const RpcValue::Array &values)
{
	writeArrayBegin(values.type(), values.size());
	for (size_t i = 0; i < values.size();) {
		write(values.valueAt(i));
		if (++i < values.size())
			separateElement();
	}
	writeContainerEnd(RpcValue::Type::Array);
	return *this;
}

CponWriter &CponWriter::write(const RpcValue::Map &values)
{
	writeContainerBegin(RpcValue::Type::Map);
	writeMapContent(values);
	writeContainerEnd(RpcValue::Type::Map);
	return *this;
}

CponWriter &CponWriter::write(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data)
{
	writeContainerBegin(RpcValue::Type::IMap);
	writeIMapContent(values, meta_data);
	writeContainerEnd(RpcValue::Type::IMap);
	return *this;
}

void CponWriter::writeIMapContent(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data)
{
	size_t ix = 0;
	for (const auto &kv : values) {
		indentElement();
		if(m_opts.setTranslateIds() && meta_data) {
			int mtid = meta_data->metaTypeId();
			int nsid = meta_data->metaTypeNameSpaceId();
			unsigned key = kv.first;
			const meta::MetaInfo key_info = meta::registeredType(nsid, mtid).keyById(key);
			if(key_info.isValid())
				m_out << key_info.name;
			else
				write(key);
		}
		else {
			write(kv.first);
		}
		m_out << ":";
		write(kv.second);
		if(++ix < values.size())
			separateElement();
	}
}

void CponWriter::writeMapContent(const RpcValue::Map &values)
{
	size_t ix = 0;
	for (const auto &kv : values) {
		indentElement();
		write(kv.first);
		m_out << ":";
		write(kv.second);
		if(++ix < values.size())
			separateElement();
	}
}

} // namespace chainpack
} // namespace shv
