#include "cponwriter.h"
#include "chainpack.h"

#include <iostream>
#include <cmath>

namespace shv {
namespace chainpack {

static constexpr bool WRITE_INVALID_AS_NULL = true;

static const std::string S_NULL("null");
static const std::string S_TRUE("true");
static const std::string S_FALSE("false");

static const std::string S_IMAP_BEGIN("i{");
static const std::string S_ARRAY_BEGIN("a[");
static const std::string S_BLOB_BEGIN("x\"");
static const std::string S_DATETIME_BEGIN("d\"");
//static const char S_LIST_BEGIN('[');
//static const char S_LIST_END(']');
//static const char S_MAP_BEGIN('{');
//static const char S_MAP_END('}');
static const char S_META_BEGIN('<');
static const char S_META_END('>');
static const char S_DECIMAL_END('n');
static const char S_UNSIGNED_END('u');

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

CponWriter &CponWriter::operator <<(const RpcValue &value)
{
	if(!value.metaData().isEmpty()) {
		*this << value.metaData();
		if(m_opts.isIndent())
			m_out << '\n';
	}
	switch (value.type()) {
	case RpcValue::Type::Null: *this << nullptr; return *this;
	case RpcValue::Type::UInt: *this << value.toUInt(); return *this;
	case RpcValue::Type::Int: *this << value.toInt(); return *this;
	case RpcValue::Type::Double: *this << value.toDouble(); return *this;
	case RpcValue::Type::Bool: *this << value.toBool(); return *this;
	case RpcValue::Type::Blob: *this << value.toBlob(); return *this;
	case RpcValue::Type::String: *this << value.toString(); return *this;
	case RpcValue::Type::DateTime: *this << value.toDateTime(); return *this;
	case RpcValue::Type::List: *this << value.toList(); return *this;
	case RpcValue::Type::Array: *this << value.toArray(); return *this;
	case RpcValue::Type::Map: *this << value.toMap(); return *this;
	case RpcValue::Type::IMap: writeIMap(value.toIMap(), &value.metaData()); return *this;
	case RpcValue::Type::Decimal: *this << value.toDecimal(); return *this;
	case RpcValue::Type::Invalid:
		if(WRITE_INVALID_AS_NULL) {
			*this << nullptr;
			return *this;
		}
	}
	SHVCHP_EXCEPTION(std::string("Don't know how to serialize type: ") + RpcValue::typeToName(value.type()));
}

CponWriter &CponWriter::operator <<(const RpcValue::MetaData &meta_data)
{
	if(!meta_data.isEmpty()) {
		*this << Begin::Meta;
		const RpcValue::IMap &cim = meta_data.iValues();
		if(!cim.empty())
			writeIMapContent(cim);
		const RpcValue::Map &csm = meta_data.sValues();
		if(!csm.empty()) {
			if(!cim.empty())
				separateElement();
			writeMapContent(csm);
		}
		*this << End::Meta;
	}
	return *this;
}

CponWriter &CponWriter::operator <<(CponWriter::Begin manip)
{
	switch (manip) {
	case Begin::List:
		m_out << '[';
		startBlock();
		break;
	case Begin::Map:
		m_out << '{';
		startBlock();
		break;
	case Begin::IMap:
		m_out << S_IMAP_BEGIN;
		startBlock();
		break;
	case Begin::Array:
		m_out << S_ARRAY_BEGIN;
		startBlock();
		break;
	case Begin::Meta:
		m_out << S_META_BEGIN;
		startBlock();
		break;
	}
	return *this;
}

CponWriter &CponWriter::operator <<(CponWriter::End manip)
{
	switch (manip) {
	case End::List:
	case End::Array:
		endBlock();
		m_out << ']';
		break;
	case End::IMap:
	case End::Map:
		endBlock();
		m_out << '}';
		break;
	case End::Meta:
		endBlock();
		m_out << S_META_END;
		break;
	}
	return *this;
}

CponWriter &CponWriter::operator <<(const CponWriter::ListElement &el)
{
	*this << el.val;
	separateElement();
	return *this;
}

CponWriter &CponWriter::operator <<(const CponWriter::MapElement &el)
{
	*this << el.key;
	m_out << ':';
	*this << el.val;
	return *this;
}

CponWriter &CponWriter::operator <<(const CponWriter::IMapElement &el)
{
	*this << el.key;
	m_out << ':';
	*this << el.val;
	return *this;
}

CponWriter &CponWriter::operator <<(std::nullptr_t)
{
	m_out << S_NULL;
	return *this;
}

CponWriter &CponWriter::operator <<(bool value)
{
	m_out << (value ? S_TRUE : S_FALSE);
	return *this;
}

CponWriter &CponWriter::operator <<(RpcValue::Int value)
{
	m_out << Utils::toString(value);
	return *this;
}

CponWriter &CponWriter::operator <<(RpcValue::UInt value)
{
	m_out << Utils::toString(value);
	m_out << S_UNSIGNED_END;
	return *this;
}

CponWriter &CponWriter::operator <<(double value)
{
	if (std::isfinite(value)) {
		m_out << value;
	}
	else {
		m_out << S_NULL;
	}
	return *this;
}

CponWriter &CponWriter::operator <<(RpcValue::Decimal value)
{
	m_out << value.toString() << S_DECIMAL_END;
	return *this;
}

CponWriter &CponWriter::operator <<(RpcValue::DateTime value)
{
	m_out << S_DATETIME_BEGIN << value.toUtcString() << '"';
	return *this;
}

CponWriter &CponWriter::operator <<(const std::string &value)
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

CponWriter &CponWriter::operator <<(const RpcValue::Blob &value)
{
	m_out << S_BLOB_BEGIN;
	m_out << Utils::toHex(value);
	m_out << '"';
	return *this;
}

CponWriter &CponWriter::operator <<(const RpcValue::List &values)
{
	*this << Begin::List;
	for (size_t i = 0; i < values.size(); ) {
		indentElement();
		const RpcValue &value = values[i];
		*this << value;
		if (++i < values.size())
			separateElement();
	}
	*this << End::List;
	return *this;
}

CponWriter &CponWriter::operator <<(const RpcValue::Array &values)
{
	m_out << S_ARRAY_BEGIN;
	for (size_t i = 0; i < values.size();) {
		*this << values.valueAt(i);
		if (++i < values.size())
			separateElement();
	}
	m_out << ']'; // array is serialized flat
	return *this;
}

CponWriter &CponWriter::operator <<(const RpcValue::Map &values)
{
	*this << Begin::Map;
	writeMapContent(values);
	*this << End::Map;
	return *this;
}

CponWriter &CponWriter::operator <<(const RpcValue::IMap &values)
{
	writeIMap(values, nullptr);
	return *this;
}

void CponWriter::writeIMap(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data)
{
	*this << Begin::IMap;
	writeIMapContent(values, meta_data);
	*this << End::IMap;
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
				*this << key;
		}
		else {
			*this << kv.first;
		}
		m_out << ":";
		*this << kv.second;
		if(++ix < values.size())
			separateElement();
	}
}

void CponWriter::writeMapContent(const RpcValue::Map &values)
{
	size_t ix = 0;
	for (const auto &kv : values) {
		indentElement();
		*this << kv.first;
		m_out << ":";
		*this << kv.second;
		if(++ix < values.size())
			separateElement();
	}
}

} // namespace chainpack
} // namespace shv
