#include "cponwriter.h"
#include "cpon.h"
#include "exception.h"

#include <iostream>
#include <cmath>
#include <sstream>

namespace shv {
namespace chainpack {

void CponWriter::startBlock()
{
	if(!m_opts.indent().empty()) {
		m_out << '\n';
		m_currentIndent++;
	}
}

void CponWriter::endBlock()
{
	if(!m_opts.indent().empty()) {
		//m_out << '\n';
		m_currentIndent--;
		indentElement();
	}
}

void CponWriter::indentElement()
{
	if(!m_opts.indent().empty()) {
		for (int i = 0; i < m_currentIndent; ++i) {
			m_out << m_opts.indent();
		}
	}
}

void CponWriter::separateElement(bool without_comma)
{
	if(m_opts.indent().empty()) {
		if(!without_comma)
			m_out << ", ";
	}
	else {
		if(!without_comma)
			m_out << ',';
		m_out << '\n';
	}
}

size_t CponWriter::write(const RpcValue &value)
{
	size_t len = m_out.tellp();
	if(!value.metaData().isEmpty()) {
		write(value.metaData());
		if(!m_opts.indent().empty())
			m_out << '\n';
	}
	switch (value.type()) {
	case RpcValue::Type::Null: write(nullptr); break;
	case RpcValue::Type::UInt: write(value.toUInt64()); break;
	case RpcValue::Type::Int: write(value.toInt64()); break;
	case RpcValue::Type::Double: write(value.toDouble()); break;
	case RpcValue::Type::Bool: write(value.toBool()); break;
	case RpcValue::Type::Blob: write(value.toBlob()); break;
	case RpcValue::Type::String: write(value.toString()); break;
	case RpcValue::Type::DateTime: write(value.toDateTime()); break;
	case RpcValue::Type::List: write(value.toList()); break;
	case RpcValue::Type::Array: write(value.toArray()); break;
	case RpcValue::Type::Map: write(value.toMap()); break;
	case RpcValue::Type::IMap: write(value.toIMap(), &value.metaData()); break;
	case RpcValue::Type::Decimal: write(value.toDecimal()); break;
	case RpcValue::Type::Invalid:
		if(WRITE_INVALID_AS_NULL) {
			write(nullptr);
		}
		break;
	}
	//SHVCHP_EXCEPTION(std::string("Don't know how to serialize type: ") + RpcValue::typeToName(value.type()));
	return (size_t)m_out.tellp() - len;
}

size_t CponWriter::write(const RpcValue::MetaData &meta_data)
{
	size_t len = m_out.tellp();
	if(!meta_data.isEmpty()) {
		m_out << Cpon::C_META_BEGIN;
		startBlock();
		const RpcValue::IMap &cim = meta_data.iValues();
		if(!cim.empty()) {
			int nsid = meta_data.metaTypeNameSpaceId();
			int mtid = meta_data.metaTypeId();
			size_t ix = 0;
			for (const auto &kv : cim) {
				indentElement();
				unsigned tag = kv.first;
				if(m_opts.isTranslateIds()) {
					const meta::MetaInfo &tag_info = meta::registeredType(nsid, mtid).tagById(tag);
					if(tag_info.isValid())
						m_out << tag_info.name;
					else
						m_out << Utils::toString(tag);
				}
				else {
					m_out << Utils::toString(tag);
				}
				m_out << ":";
				RpcValue meta_val = kv.second;
				if(m_opts.isTranslateIds()) {
					if(tag == meta::Tag::MetaTypeNameSpaceId) {
						int id = meta_val.toInt();
						const meta::MetaNameSpace &type = meta::registeredNameSpace(nsid);
						const char *n = type.name();
						if(n[0])
							m_out << n;
						else
							m_out << Utils::toString(id);
					}
					else if(tag == meta::Tag::MetaTypeId) {
						int id = meta_val.toInt();
						const meta::MetaType &type = meta::registeredType(nsid, id);
						const char *n = type.name();
						if(n[0])
							m_out << n;
						else
							m_out << Utils::toString(id);
					}
					else {
						write(meta_val);
					}
				}
				else {
					write(meta_val);
				}
				separateElement(++ix == cim.size());
			}
		}
		const RpcValue::Map &csm = meta_data.sValues();
		if(!csm.empty()) {
			if(!cim.empty())
				separateElement(false);
			writeMapContent(csm);
		}
		endBlock();
		m_out << Cpon::C_META_END;
	}
	return (size_t)m_out.tellp() - len;
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

void CponWriter::writeListElement(const RpcValue &val, bool without_separator)
{
	indentElement();
	write(val);
	separateElement(without_separator);
}

void CponWriter::writeMapElement(const std::string &key, const RpcValue &val, bool without_separator)
{
	indentElement();
	write(key);
	m_out << ':';
	write(val);
	separateElement(without_separator);
}

void CponWriter::writeMapElement(RpcValue::UInt key, const RpcValue &val, bool without_separator)
{
	indentElement();
	write(key);
	m_out << ':';
	write(val);
	separateElement(without_separator);
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

CponWriter &CponWriter::write(int32_t value)
{
	m_out << Utils::toString(value);
	return *this;
}

CponWriter &CponWriter::write(uint32_t value)
{
	m_out << Utils::toString(value);
	if(!m_opts.isJsonFormat())
		m_out << Cpon::C_UNSIGNED_END;
	return *this;
}

CponWriter &CponWriter::write(int64_t value)
{
	m_out << Utils::toString(value);
	return *this;
}

CponWriter &CponWriter::write(uint64_t value)
{
	m_out << Utils::toString(value);
	if(!m_opts.isJsonFormat())
		m_out << Cpon::C_UNSIGNED_END;
	return *this;
}

CponWriter &CponWriter::write(double value)
{
	std::ostringstream ss;
	if (std::isfinite(value)) {
		ss << value;
	}
	else {
		ss << Cpon::STR_NULL;
	}
	std::string s = ss.str();
	if(s.find('.') == std::string::npos)
		s += '.';
	m_out << s;
	return *this;
}

CponWriter &CponWriter::write(RpcValue::Decimal value)
{
	m_out << value.toString() << Cpon::C_DECIMAL_END;
	return *this;
}

CponWriter &CponWriter::write(RpcValue::DateTime value)
{
	m_out << Cpon::STR_DATETIME_BEGIN << value.toIsoString() << '"';
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
	if(m_opts.isHexBlob()) {
		m_out << Cpon::STR_HEX_BLOB_BEGIN;
		m_out << Utils::toHex(value);
		m_out << '"';
	}
	else {
		m_out << Cpon::STR_ESC_BLOB_BEGIN;
		for (size_t i = 0; i < value.length(); i++) {
			const char ch = value[i];
			switch (ch) {
			case '\\': m_out << "\\\\"; break;
			case '"' : m_out << "\\\""; break;
			case '\b': m_out << "\\b"; break;
			case '\f': m_out << "\\f"; break;
			case '\n': m_out << "\\n"; break;
			case '\r': m_out << "\\r"; break;
			case '\t': m_out << "\\t"; break;
			default: {
				if (static_cast<uint8_t>(ch) <= 0x1f) {
					char buf[8];
					snprintf(buf, sizeof buf, "\\x%02x", ch);
					m_out << buf;
				}
				else {
					m_out << ch;
				}
			}
			}
		}
		m_out << '"';
	}
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

CponWriter &CponWriter::write(const RpcValue::List &values)
{
	writeContainerBegin(RpcValue::Type::List);
	for (size_t ix = 0; ix < values.size(); ) {
		const RpcValue &value = values[ix];
		writeListElement(value, ++ix == values.size());
	}
	writeContainerEnd(RpcValue::Type::List);
	return *this;
}

CponWriter &CponWriter::write(const RpcValue::Array &values)
{
	writeArrayBegin(values.type(), values.size());
	for (size_t ix = 0; ix < values.size();) {
		RpcValue v = values.valueAt(ix);
		writeArrayElement(v, ++ix == values.size());
	}
	writeContainerEnd(RpcValue::Type::Array);
	return *this;
}

void CponWriter::writeIMapContent(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data)
{
	size_t ix = 0;
	for (const auto &kv : values) {
		indentElement();
		if(m_opts.isTranslateIds() && meta_data) {
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
		separateElement(++ix == values.size());
	}
}

void CponWriter::writeMapContent(const RpcValue::Map &values)
{
	size_t ix = 0;
	for (const auto &kv : values)
		writeMapElement(kv.first, kv.second, ++ix == values.size());
}

} // namespace chainpack
} // namespace shv
