#include "cponprotocol.h"

#include "utils.h"

#include <limits>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <iostream>

namespace shv {
namespace chainpack {
#if 0
static constexpr bool WRITE_INVALID_AS_NULL = true;
#endif

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

//==============================================================
// Parsing
//==============================================================
enum {exception_aborts = 0};
//enum {exception_aborts = 1};

#define PARSE_EXCEPTION(msg) {\
	std::clog << __FILE__ << ':' << __LINE__; \
	if(exception_aborts) { \
		std::clog << ' ' << (msg) << std::endl; \
		abort(); \
	} \
	else { \
		std::clog << ' ' << (msg) << std::endl; \
		throw CponProtocol::ParseException(msg, m_pos); \
	} \
}

static const int MAX_RECURSION_DEPTH = 1000;

static inline std::string dump_char(char c)
{
	char buf[12];
	if (static_cast<uint8_t>(c) >= 0x20 && static_cast<uint8_t>(c) <= 0x7f) {
		snprintf(buf, sizeof buf, "'%c' (%d)", c, c);
	}
	else {
		snprintf(buf, sizeof buf, "(%d)", c);
	}
	return std::string(buf);
}

static inline bool in_range(long x, long lower, long upper)
{
	return (x >= lower && x <= upper);
}

static inline bool starts_with(const std::string &str, size_t pos, const std::string &prefix)
{
	return std::equal(prefix.begin(), prefix.end(), str.begin() + pos);
}

static inline bool starts_with(const std::string &str, size_t pos, const char prefix)
{
	return (str.begin() + pos) != str.end() && *(str.begin() + pos) == prefix;
}

CponProtocol::CponProtocol(const std::string &str, size_t pos)
	: m_str(str)
	, m_pos(pos)
{
}

RpcValue CponProtocol::read(const std::string &in, size_t pos, size_t *new_pos)
{
	CponProtocol parser{in, pos};
	RpcValue result = parser.parseAtPos();
	if(new_pos)
		*new_pos = parser.pos();
	return result;
}

RpcValue::MetaData CponProtocol::readMetaData(const std::string &in, size_t pos, size_t *new_pos)
{
	RpcValue::MetaData md;
	CponProtocol parser{in, pos};
	bool ok = parser.parseMetaData(md);
	if(new_pos)
		*new_pos = ok? parser.pos(): pos;
	return md;
}

namespace {
class DepthScope
{
public:
	DepthScope(int &depth) : m_depth(depth) {m_depth++;}
	~DepthScope() {m_depth--;}
private:
	int &m_depth;
};
}
RpcValue CponProtocol::parseAtPos()
{
	if (m_depth > MAX_RECURSION_DEPTH)
		PARSE_EXCEPTION("maximum nesting depth exceeded");
	DepthScope{m_depth};

	RpcValue ret_val;
	RpcValue::MetaData meta_data;

	skipGarbage();
	if(parseMetaData(meta_data))
		skipGarbage();
	do {
		if(parseString(ret_val)) { break; }
		else if(parseNumber(ret_val)) { break; }
		else if(parseNull(ret_val)) { break; }
		else if(parseBool(ret_val)) { break; }
		else if(parseList(ret_val)) { break; }
		else if(parseMap(ret_val)) { break; }
		else if(parseIMap(ret_val)) { break; }
		else if(parseArray(ret_val)) { break; }
		else if(parseBlob(ret_val)) { break; }
		else if(parseDateTime(ret_val)) { break; }
		else
			PARSE_EXCEPTION("unknown type: " + m_str.substr(m_pos, 40));
	} while(false);
	if(!meta_data.isEmpty())
		ret_val.setMetaData(std::move(meta_data));
	return ret_val;
}

void CponProtocol::skipWhiteSpace()
{
	while (m_str[m_pos] == ' ' || m_str[m_pos] == '\r' || m_str[m_pos] == '\n' || m_str[m_pos] == '\t')
		m_pos++;
}

bool CponProtocol::skipComment()
{
	bool comment_found = false;
	if (m_str[m_pos] == '/') {
		m_pos++;
		if (m_pos == m_str.size())
			PARSE_EXCEPTION("unexpected end of input after start of comment");
		if (m_str[m_pos] == '/') { // inline comment
			m_pos++;
			// advance until next line, or end of input
			while (m_pos < m_str.size() && m_str[m_pos] != '\n') {
				m_pos++;
			}
			comment_found = true;
		}
		else if (m_str[m_pos] == '*') { // multiline comment
			m_pos++;
			if (m_pos > m_str.size()-2)
				PARSE_EXCEPTION("unexpected end of input inside multi-line comment");
			// advance until closing tokens
			while (!(m_str[m_pos] == '*' && m_str[m_pos+1] == '/')) {
				m_pos++;
				if (m_pos > m_str.size()-2)
					PARSE_EXCEPTION( "unexpected end of input inside multi-line comment");
			}
			m_pos += 2;
			comment_found = true;
		}
		else {
			PARSE_EXCEPTION("malformed comment");
		}
	}
	return comment_found;
}

char CponProtocol::skipGarbage()
{
	if (m_pos >= m_str.size())
		PARSE_EXCEPTION("unexpected end of input");
	skipWhiteSpace();
	{
		bool comment_found = false;
		do {
			comment_found = skipComment();
			skipWhiteSpace();
		}
		while(comment_found);
	}
	return m_str[m_pos];
}

char CponProtocol::nextValidChar()
{
	m_pos++;
	skipGarbage();
	return m_str[m_pos];
}

char CponProtocol::currentChar()
{
	if (m_pos >= m_str.size())
		PARSE_EXCEPTION("unexpected end of input");
	return m_str[m_pos];
}

bool CponProtocol::parseMetaData(RpcValue::MetaData &meta_data)
{
	char ch = m_str[m_pos];
	if(ch != S_META_BEGIN)
		return false;
	ch = nextValidChar();
	meta_data = parseMetaDataContent(S_META_END);
	return true;
}

bool CponProtocol::parseNull(RpcValue &val)
{
	if(starts_with(m_str, m_pos, S_NULL)) {
		m_pos += S_NULL.size();
		val = RpcValue(nullptr);
		return true;
	}
	return false;
}

bool CponProtocol::parseBool(RpcValue &val)
{
	if(starts_with(m_str, m_pos, S_TRUE)) {
		m_pos += S_TRUE.size();
		val = RpcValue(true);
		return true;
	}
	if(starts_with(m_str, m_pos, S_FALSE)) {
		m_pos += S_FALSE.size();
		val = RpcValue(false);
		return true;
	}
	return false;
}

bool CponProtocol::parseString(RpcValue &val)
{
	std::string s;
	if(parseStringHelper(s)) {
		val = s;
		return true;
	}
	return false;
}

bool CponProtocol::parseBlob(RpcValue &val)
{
	char ch = currentChar();
	if (ch != 'x')
		return false;
	m_pos++;
	std::string s;
	if(parseStringHelper(s)) {
		s = Utils::fromHex(s);
		val = RpcValue::Blob{s};
		return true;
	}
	return false;
}

bool CponProtocol::parseNumber(RpcValue &val)
{
	int sign = 1;
	uint64_t int_part = 0;
	uint64_t dec_part = 0;
	int dec_cnt = 0;
	int radix = 10;
	int exponent = std::numeric_limits<int>::max();
	RpcValue::Type type = RpcValue::Type::Invalid;

	bool is_number = false;
	char ch = currentChar();
	if (ch == '-') {
		sign = -1;
		ch = nextValidChar();
	}
	if (starts_with(m_str, m_pos, "0x")) {
		radix = 16;
		nextValidChar();
		ch = nextValidChar();
		is_number = true;
	}
	else if (in_range(ch, '0', '9')) {
		is_number = true;
	}
	if(!is_number)
		return false;
	if (in_range(ch, '0', '9')) {
		type = RpcValue::Type::Int;
		size_t start_pos = m_pos;
		int_part = parseDecimalUnsigned(radix);
		if (m_pos == start_pos)
			PARSE_EXCEPTION("number integer part missing");
		if (starts_with(m_str, m_pos, S_UNSIGNED_END)) {
			type = RpcValue::Type::UInt;
			m_pos++;
		}
	}
	if (starts_with(m_str, m_pos, '.')) {
		m_pos++;
		size_t start_pos = m_pos;
		dec_part = parseDecimalUnsigned(radix);
		type = RpcValue::Type::Double;
		dec_cnt = m_pos - start_pos;
		if (starts_with(m_str, m_pos, S_DECIMAL_END)) {
			type = RpcValue::Type::Decimal;
			m_pos++;
		}
	}
	else if(starts_with(m_str, m_pos, 'e') || starts_with(m_str, m_pos, 'E')) {
		bool neg = false;
		if (starts_with(m_str, m_pos, '-')) {
			neg = true;
			m_pos++;
		}
		size_t start_pos = m_pos;
		exponent = parseDecimalUnsigned(radix);
		if (m_pos == start_pos)
			PARSE_EXCEPTION("double exponent part missing");
		exponent *= neg;
		type = RpcValue::Type::Double;
	}

	switch (type) {
	case RpcValue::Type::Int:
		val = RpcValue((RpcValue::Int)(int_part * sign));
		break;
	case RpcValue::Type::UInt:
		val = RpcValue((RpcValue::UInt)int_part);
		break;
	case RpcValue::Type::Decimal: {
		int64_t n = int_part * sign;
		for (int i = 0; i < dec_cnt; ++i)
			n *= 10;
		n += dec_part;
		val = RpcValue(RpcValue::Decimal(n, dec_cnt));
		break;
	}
	case RpcValue::Type::Double: {
		double d = 0;
		if(exponent == std::numeric_limits<int>::max()) {
			d = dec_part;
			for (int i = 0; i < dec_cnt; ++i)
				d /= 10;
			d += int_part;
		}
		else {
			d = int_part;
			d *= std::pow(10, exponent);
		}
		val = RpcValue(d * sign);
		break;
	}
	default:
		PARSE_EXCEPTION("number parse error");
	}
	return true;
}

bool CponProtocol::parseList(RpcValue &val)
{
	char ch = currentChar();
	if (ch != '[')
		return false;
	ch = nextValidChar();
	RpcValue::List lst;
	while (true) {
		if (ch == ']')
			break;
		lst.push_back(parseAtPos());
		ch = skipGarbage();
		if (ch == ',')
			ch = nextValidChar();
		//	PARSE_EXCEPTION("expected ',' in list, got " + dump_char(ch));
	}
	m_pos++;
	val = lst;
	return true;
}

bool CponProtocol::parseMap(RpcValue &val)
{
	char ch = currentChar();
	if (ch != '{')
		return false;
	ch = nextValidChar();
	RpcValue::Map map;
	while (true) {
		if (ch == '}')
			break;
		std::string key;
		if(!parseStringHelper(key))
			PARSE_EXCEPTION("expected string key, got " + dump_char(ch));
		ch = skipGarbage();
		if (ch != ':')
			PARSE_EXCEPTION("expected ':' in Map, got " + dump_char(ch));
		m_pos++;
		RpcValue key_val = parseAtPos();
		map[key] = key_val;
		ch = skipGarbage();
		if (ch == ',')
			ch = nextValidChar();
		//PARSE_EXCEPTION("unexpected delimiter in IMap, got " + dump_char(ch));
	}
	m_pos++;
	val = map;
	return true;
}

bool CponProtocol::parseIMap(RpcValue &val)
{
	if (!starts_with(m_str, m_pos, S_IMAP_BEGIN))
		return false;
	m_pos += S_IMAP_BEGIN.size();
	skipGarbage();
	RpcValue::IMap imap = parseIMapContent('}');
	val = imap;
	return true;
}

bool CponProtocol::parseArray(RpcValue &ret_val)
{
	if(!starts_with(m_str, m_pos, S_ARRAY_BEGIN))
		return false;

	m_pos += S_ARRAY_BEGIN.size();
	RpcValue::Array arr;
	char ch = skipGarbage();
	while (true) {
		if (ch == ']')
			break;
		RpcValue val = parseAtPos();
		if(arr.empty()) {
			arr = RpcValue::Array(val.type());
		}
		else {
			if(val.type() != arr.type())
				PARSE_EXCEPTION("Mixed types in Array");
		}
		arr.push_back(RpcValue::Array::makeElement(val));
		ch = skipGarbage();
		if (ch == ',')
			ch = nextValidChar();
	}
	m_pos++;
	ret_val = arr;
	return true;
}

bool CponProtocol::parseDateTime(RpcValue &val)
{
	if(!starts_with(m_str, m_pos, S_DATETIME_BEGIN))
		return false;
	m_pos += 1;
	std::string s;
	if(parseStringHelper(s)) {
		val = RpcValue::DateTime::fromUtcString(s);
		return true;
	}
	PARSE_EXCEPTION("error parsing DateTime");
}

uint64_t CponProtocol::parseDecimalUnsigned(int radix)
{
	uint64_t ret = 0;
	while (m_pos < m_str.size()) {
		char ch = m_str[m_pos];
		if(in_range(ch, '0', '9')) {
			ret *= radix;
			ret += (ch - '0');
			m_pos++;
		}
		else {
			break;
		}
	}
	return ret;
}

RpcValue::IMap CponProtocol::parseIMapContent(char closing_bracket)
{
	RpcValue::IMap map;
	char ch = skipGarbage();
	while (true) {
		if(ch == closing_bracket)
			break;
		RpcValue v;
		if(!parseNumber(v))
			PARSE_EXCEPTION("number key expected");
		if(!(v.type() == RpcValue::Type::Int || v.type() == RpcValue::Type::UInt))
			PARSE_EXCEPTION("int key expected");
		RpcValue::UInt key = v.toUInt();
		ch = skipGarbage();
		if (ch != ':')
			PARSE_EXCEPTION("expected ':' in IMap, got " + dump_char(ch));
		m_pos++;
		RpcValue val = parseAtPos();
		map[key] = val;
		ch = skipGarbage();
		if (ch == ',')
			ch = nextValidChar();
	}
	m_pos++;
	return map;
}

RpcValue::MetaData CponProtocol::parseMetaDataContent(char closing_bracket)
{
	RpcValue::IMap imap;
	RpcValue::Map smap;
	char ch = skipGarbage();
	while (true) {
		if(ch == closing_bracket)
			break;
		RpcValue key = parseAtPos();
		if(!(key.type() == RpcValue::Type::Int || key.type() == RpcValue::Type::UInt)
		   && !(key.type() == RpcValue::Type::String))
			PARSE_EXCEPTION("key expected");
		ch = skipGarbage();
		if (ch != ':')
			PARSE_EXCEPTION("expected ':' in IMap, got " + dump_char(ch));
		nextValidChar();
		RpcValue val = parseAtPos();
		if(key.type() == RpcValue::Type::String)
			smap[key.toString()] = val;
		else
			imap[key.toUInt()] = val;
		ch = skipGarbage();
		if (ch == ',')
			ch = nextValidChar();
	}
	m_pos++;
	return RpcValue::MetaData(std::move(imap), std::move(smap));
}

bool CponProtocol::parseStringHelper(std::string &val)
{
	char ch = currentChar();
	if (ch != '"')
		return false;

	m_pos++;
	std::string str_val;
	long last_escaped_codepoint = -1;
	while (true) {
		if (m_pos >= m_str.size())
			PARSE_EXCEPTION("unexpected end of input in string");

		ch = m_str[m_pos++];

		if (ch == '"') {
			encodeUtf8(last_escaped_codepoint, str_val);
			val = str_val;
			return true;
		}

		if (in_range(ch, 0, 0x1f))
			PARSE_EXCEPTION("unescaped " + dump_char(ch) + " in string");

		// The usual case: non-escaped characters
		if (ch != '\\') {
			encodeUtf8(last_escaped_codepoint, str_val);
			last_escaped_codepoint = -1;
			str_val += ch;
			continue;
		}

		// Handle escapes
		if (m_pos == m_str.size())
			PARSE_EXCEPTION("unexpected end of input in string");

		ch = m_str[m_pos++];

		if (ch == 'u') {
			// Extract 4-byte escape sequence
			std::string esc = m_str.substr(m_pos, 4);
			// Explicitly check length of the substring. The following loop
			// relies on std::string returning the terminating NUL when
			// accessing str[length]. Checking here reduces brittleness.
			if (esc.length() < 4) {
				PARSE_EXCEPTION("bad \\u escape: " + esc);
			}
			for (size_t j = 0; j < 4; j++) {
				if (!in_range(esc[j], 'a', 'f') && !in_range(esc[j], 'A', 'F')
					&& !in_range(esc[j], '0', '9'))
					PARSE_EXCEPTION("bad \\u escape: " + esc);
			}

			long codepoint = strtol(esc.data(), nullptr, 16);

			// JSON specifies that characters outside the BMP shall be encoded as a pair
			// of 4-hex-digit \u escapes encoding their surrogate pair components. Check
			// whether we're in the middle of such a beast: the previous codepoint was an
			// escaped lead (high) surrogate, and this is a trail (low) surrogate.
			if (in_range(last_escaped_codepoint, 0xD800, 0xDBFF)
				&& in_range(codepoint, 0xDC00, 0xDFFF)) {
				// Reassemble the two surrogate pairs into one astral-plane character, per
				// the UTF-16 algorithm.
				encodeUtf8((((last_escaped_codepoint - 0xD800) << 10)
							 | (codepoint - 0xDC00)) + 0x10000, str_val);
				last_escaped_codepoint = -1;
			} else {
				encodeUtf8(last_escaped_codepoint, str_val);
				last_escaped_codepoint = codepoint;
			}

			m_pos += 4;
			continue;
		}

		encodeUtf8(last_escaped_codepoint, str_val);
		last_escaped_codepoint = -1;

		if (ch == 'b') {
			str_val += '\b';
		} else if (ch == 'f') {
			str_val += '\f';
		} else if (ch == 'n') {
			str_val += '\n';
		} else if (ch == 'r') {
			str_val += '\r';
		} else if (ch == 't') {
			str_val += '\t';
		} else if (ch == '"' || ch == '\\' || ch == '/') {
			str_val += ch;
		} else {
			PARSE_EXCEPTION("invalid escape character " + dump_char(ch));
		}
	}
}

void CponProtocol::encodeUtf8(long pt, std::string & out)
{
	if (pt < 0)
		return;

	if (pt < 0x80) {
		out += static_cast<char>(pt);
	} else if (pt < 0x800) {
		out += static_cast<char>((pt >> 6) | 0xC0);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	} else if (pt < 0x10000) {
		out += static_cast<char>((pt >> 12) | 0xE0);
		out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	} else {
		out += static_cast<char>((pt >> 18) | 0xF0);
		out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
		out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	}
}

//==============================================================
// Serialization
//==============================================================
#if 0
void CponProtocol::write(std::ostream &out, const RpcValue &value, const WriteOptions &opts)
{
	if(!value.metaData().isEmpty())
		write(value.metaData(), out, opts);
	switch (value.type()) {
	case RpcValue::Type::Null: write(nullptr, out); break;
	case RpcValue::Type::UInt: write(value.toUInt(), out); break;
	case RpcValue::Type::Int: write(value.toInt(), out); break;
	case RpcValue::Type::Double: write(value.toDouble(), out); break;
	case RpcValue::Type::Bool: write(value.toBool(), out); break;
	case RpcValue::Type::Blob: write(value.toBlob(), out); break;
	case RpcValue::Type::String: write(value.toString(), out); break;
	case RpcValue::Type::DateTime: write(value.toDateTime(), out); break;
	case RpcValue::Type::List: write(value.toList(), out); break;
	case RpcValue::Type::Array: write(value.toArray(), out); break;
	case RpcValue::Type::Map: write(value.toMap(), out); break;
	case RpcValue::Type::IMap: write(value.toIMap(), out, opts, value.metaData()); break;
	//case RpcValue::Type::MetaIMap: serialize(value.toMetaIMap(), out); break;
	case RpcValue::Type::Decimal: write(value.toDecimal(), out); break;
	case RpcValue::Type::Invalid:
		if(WRITE_INVALID_AS_NULL) {
			write(nullptr, out);
			break;
		}
	default:
		SHVCHP_EXCEPTION(std::string("Don't know how to serialize type: ") + RpcValue::typeToName(value.type()));
	}
}

void CponProtocol::writeMetaData(std::ostream &out, const RpcValue::MetaData &meta_data, const CponProtocol::WriteOptions &opts)
{
	if(!meta_data.isEmpty())
		write(meta_data, out, opts);
}

void CponProtocol::write(std::nullptr_t, std::ostream &out)
{
	out << S_NULL;
}

void CponProtocol::write(double value, std::ostream &out)
{
	if (std::isfinite(value)) {
		char buf[32];
		snprintf(buf, sizeof buf, "%.17g", value);
		out << buf;
	}
	else {
		out << S_NULL;
	}
}

void CponProtocol::write(RpcValue::Int value, std::ostream &out)
{
	out << Utils::toString(value);
}

void CponProtocol::write(RpcValue::UInt value, std::ostream &out)
{
	out << Utils::toString(value);
}

void CponProtocol::write(bool value, std::ostream &out)
{
	out << (value ? S_TRUE : S_FALSE);
}

void CponProtocol::write(RpcValue::DateTime value, std::ostream &out)
{
	out << S_DATETIME_BEGIN << value.toUtcString() << '"';
}

void CponProtocol::write(RpcValue::Decimal value, std::ostream &out)
{
	out << value.toString() << S_DECIMAL_END;
}

void CponProtocol::write(const std::string &value, std::ostream &out)
{
	out << '"';
	for (size_t i = 0; i < value.length(); i++) {
		const char ch = value[i];
		if (ch == '\\') {
			out << "\\\\";
		} else if (ch == '"') {
			out << "\\\"";
		} else if (ch == '\b') {
			out << "\\b";
		} else if (ch == '\f') {
			out << "\\f";
		} else if (ch == '\n') {
			out << "\\n";
		} else if (ch == '\r') {
			out << "\\r";
		} else if (ch == '\t') {
			out << "\\t";
		} else if (static_cast<uint8_t>(ch) <= 0x1f) {
			char buf[8];
			snprintf(buf, sizeof buf, "\\u%04x", ch);
			out << buf;
		} else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
				 && static_cast<uint8_t>(value[i+2]) == 0xa8) {
			out << "\\u2028";
			i += 2;
		} else if (static_cast<uint8_t>(ch) == 0xe2 && static_cast<uint8_t>(value[i+1]) == 0x80
				 && static_cast<uint8_t>(value[i+2]) == 0xa9) {
			out << "\\u2029";
			i += 2;
		} else {
			out << ch;
		}
	}
	out << '"';
}

void CponProtocol::write(const RpcValue::Blob &value, std::ostream &out)
{
	out << S_BLOB_BEGIN;
	out << Utils::toHex(value);
	out << '"';
}

void CponProtocol::write(const RpcValue::List &values, std::ostream &out)
{
	bool first = true;
	out << "[";
	for (const auto &value : values) {
		if (!first)
			out << ", ";
		write(out, value);
		first = false;
	}
	out << "]";
}

void CponProtocol::write(const RpcValue::Array &values, std::ostream &out)
{
	out << S_ARRAY_BEGIN;
	for (size_t i = 0; i < values.size(); ++i) {
		if (i > 0)
			out << ", ";
		write(out, values.valueAt(i));
	}
	out << "]";
}

void CponProtocol::write(const RpcValue::Map &values, std::ostream &out)
{
	bool first = true;
	out << "{";
	for (const auto &kv : values) {
		if (!first)
			out << ", ";
		write(kv.first, out);
		out << ":";
		write(out, kv.second);
		first = false;
	}
	out << "}";
}

void CponProtocol::write(const RpcValue::IMap &values, std::ostream &out, const WriteOptions &opts, const RpcValue::MetaData &meta_data)
{
	bool first = true;
	out << S_IMAP_BEGIN;
	for (const auto &kv : values) {
		if (!first)
			out << ", ";
		if(opts.translateIds() && !meta_data.isEmpty()) {
			int mtid = meta_data.metaTypeId();
			int nsid = meta_data.metaTypeNameSpaceId();
			unsigned key = kv.first;
			const meta::MetaInfo key_info = meta::registeredType(nsid, mtid).keyById(key);
			if(key_info.isValid())
				out << key_info.name;
			else
				out << key;
		}
		else {
			write(kv.first, out);
		}
		out << ":";
		write(out, kv.second);
		first = false;
	}
	out << "}";
}

void CponProtocol::write(const RpcValue::MetaData &value, std::ostream &out, const CponProtocol::WriteOptions &opts)
{
	int n = 0;
	int nsid = value.metaTypeNameSpaceId();
	int mtid = value.metaTypeId();

	out << S_META_BEGIN;
	for(auto tag : value.iKeys()) {
		//if(key == RpcValue::Tag::MetaTypeId || key == RpcValue::Tag::MetaTypeNameSpaceId)
		//	continue;
		if(n++ > 0)
			out << ", ";
		if(opts.translateIds()) {
			const meta::MetaInfo &tag_info = meta::registeredType(nsid, mtid).tagById(tag);
			if(tag_info.isValid())
				out << tag_info.name;
			else
				out << Utils::toString(tag);
		}
		else {
			out << Utils::toString(tag);
		}
		out << ':';
		RpcValue meta_val = value.value(tag);
		if(opts.translateIds()) {
			if(tag == meta::Tag::MetaTypeNameSpaceId) {
				int id = meta_val.toInt();
				const meta::MetaNameSpace &type = meta::registeredNameSpace(nsid);
				const char *n = type.name();
				if(n[0])
					out << n;
				else
					out << Utils::toString(id);
			}
			else if(tag == meta::Tag::MetaTypeId) {
				int id = meta_val.toInt();
				const meta::MetaType &type = meta::registeredType(nsid, id);
				const char *n = type.name();
				if(n[0])
					out << n;
				else
					out << Utils::toString(id);
			}
			else {
				write(out, meta_val);
			}
		}
		else {
			write(out, meta_val);
		}
	}
	for(auto tag : value.sKeys()) {
		if(n++ > 0)
			out << ", ";
		out << '"' << tag << '"';
		out << ':';
		RpcValue meta_val = value.value(tag);
		write(out, meta_val);
	}
	out << S_META_END;
}
#endif
}}
