#include "cponreader.h"

#include <iostream>
#include <cmath>

namespace shv {
namespace chainpack {

#define PARSE_EXCEPTION(msg) {\
	std::clog << __FILE__ << ':' << __LINE__; \
	if(exception_aborts) { \
		std::clog << ' ' << (msg) << std::endl; \
		abort(); \
	} \
	else { \
		std::clog << ' ' << (msg) << std::endl; \
		throw CponReader::ParseException(msg); \
	} \
}

namespace {
enum {exception_aborts = 0};

static const char* S_NULL("null");
static const char* S_TRUE("true");
static const char* S_FALSE("false");

static const char* S_IMAP_BEGIN("i{");
static const char* S_ARRAY_BEGIN("a[");
static const char* S_BLOB_BEGIN("x\"");
static const char* S_DATETIME_BEGIN("d\"");
//static const char S_LIST_BEGIN('[');
//static const char S_LIST_END(']');
//static const char S_MAP_BEGIN('{');
//static const char S_MAP_END('}');
static const char S_META_BEGIN('<');
static const char S_META_END('>');
static const char S_DECIMAL_END('n');
static const char S_UNSIGNED_END('u');

static const int MAX_RECURSION_DEPTH = 1000;

class DepthScope
{
public:
	DepthScope(int &depth) : m_depth(depth) {m_depth++;}
	~DepthScope() {m_depth--;}
private:
	int &m_depth;
};

static inline bool in_range(long x, long lower, long upper)
{
	return (x >= lower && x <= upper);
}

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

}

CponReader &CponReader::operator >>(RpcValue &value)
{
	value = parseAtPos();
	return *this;
}

CponReader &CponReader::operator >>(RpcValue::MetaData &meta_data)
{
	parseMetaData(meta_data);
	return *this;
}

RpcValue CponReader::parseAtPos()
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
			PARSE_EXCEPTION("unknown type");
	} while(false);
	if(!meta_data.isEmpty())
		ret_val.setMetaData(std::move(meta_data));
	return ret_val;
}

bool CponReader::getString(const char *str)
{
	bool ret = true;
	int cnt;
	for (cnt = 0; str[cnt]; ++cnt) {
		if(m_in.eof()) {
			ret = false;
			break;
		}
		auto ch = m_in.peek();
		if(ch != str[cnt]) {
			ret = false;
			break;
		}
		m_in.get();
	}
	if(!ret)
		while (cnt--)
			m_in.unget();
	return ret;
}

void CponReader::skipWhiteSpace()
{
	do {
		auto ch = m_in.peek();
		if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t') {
			m_in.get();
		}
	} while(true);
}

bool CponReader::skipComment()
{
	bool comment_found = false;
	if(getString("//")) {
		m_in.get(); m_in.get();
		// advance until next line, or end of input
		while(!m_in.eof()) {
			auto ch = m_in.get();
			if(ch == '\n')
				break;
		}
		comment_found = true;
	}
	else if(getString("/*")) {
		m_in.get(); m_in.get();
		int ch, prev_char = -1;
		while(!m_in.eof()) {
			if(prev_char < 0) {
				prev_char = m_in.get();
			}
			else {
				ch = m_in.get();
				if(prev_char == '*' && ch == '/')
					break;
			}
		}
		if(!(prev_char == '*' && ch == '/'))
			PARSE_EXCEPTION("unexpected end of input inside multi-line comment");
		comment_found = true;
	}
	return comment_found;
}

char CponReader::skipGarbage()
{
	skipWhiteSpace();
	{
		bool comment_found = false;
		do {
			comment_found = skipComment();
			skipWhiteSpace();
		}
		while(comment_found);
	}
	return m_in.peek();
}

char CponReader::nextValidChar()
{
	if(m_in.eof())
		PARSE_EXCEPTION("unexpected end of input");
	m_in.get();
	skipGarbage();
	return m_in.peek();
}

char CponReader::currentChar()
{
	if(m_in.eof())
		PARSE_EXCEPTION("unexpected end of input");
	return m_in.peek();
}

void CponReader::encodeUtf8(long pt, std::string &out)
{
	if (pt < 0)
		return;

	if (pt < 0x80) {
		out += static_cast<char>(pt);
	}
	else if (pt < 0x800) {
		out += static_cast<char>((pt >> 6) | 0xC0);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	}
	else if (pt < 0x10000) {
		out += static_cast<char>((pt >> 12) | 0xE0);
		out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	}
	else {
		out += static_cast<char>((pt >> 18) | 0xF0);
		out += static_cast<char>(((pt >> 12) & 0x3F) | 0x80);
		out += static_cast<char>(((pt >> 6) & 0x3F) | 0x80);
		out += static_cast<char>((pt & 0x3F) | 0x80);
	}
}

bool CponReader::parseMetaData(RpcValue::MetaData &meta_data)
{
	char ch = currentChar();
	if(ch != S_META_BEGIN)
		return false;
	ch = nextValidChar();
	meta_data = parseMetaDataContent(S_META_END);
	return true;
}

bool CponReader::parseNull(RpcValue &val)
{
	if(getString(S_NULL)) {
		val = RpcValue(nullptr);
		return true;
	}
	return false;
}

bool CponReader::parseBool(RpcValue &val)
{
	if(getString(S_TRUE)) {
		val = RpcValue(true);
		return true;
	}
	if(getString(S_FALSE)) {
		val = RpcValue(false);
		return true;
	}
	return false;
}

bool CponReader::parseString(RpcValue &val)
{
	std::string s;
	if(parseStringHelper(s)) {
		val = s;
		return true;
	}
	return false;
}

bool CponReader::parseBlob(RpcValue &val)
{
	if(!getString(S_BLOB_BEGIN))
		return false;
	m_in.unget();
	std::string s;
	if(parseStringHelper(s)) {
		s = Utils::fromHex(s);
		val = RpcValue::Blob{s};
		return true;
	}
	return false;
}

bool CponReader::parseNumber(RpcValue &val)
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
	if (getString("0x")) {
		radix = 16;
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
		auto start_pos = m_in.tellg();
		int_part = parseDecimalUnsigned(radix);
		if (m_in.tellg() == start_pos)
			PARSE_EXCEPTION("number integer part missing");
		if (currentChar() == S_UNSIGNED_END) {
			type = RpcValue::Type::UInt;
			m_in.get();
		}
	}
	if (currentChar() == '.') {
		m_in.get();
		auto start_pos = m_in.tellg();
		dec_part = parseDecimalUnsigned(radix);
		type = RpcValue::Type::Double;
		dec_cnt = m_in.tellg() - start_pos;
		if (currentChar() == S_DECIMAL_END) {
			type = RpcValue::Type::Decimal;
			m_in.get();
		}
	}
	else if(currentChar() == 'e' || currentChar() == 'E') {
		bool neg = false;
		char ch = nextValidChar();
		if (ch == '-') {
			neg = true;
			nextValidChar();
		}
		auto start_pos = m_in.tellg();
		exponent = parseDecimalUnsigned(radix);
		if (m_in.tellg() == start_pos)
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

bool CponReader::parseList(RpcValue &val)
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
	m_in.get();
	val = lst;
	return true;
}

bool CponReader::parseMap(RpcValue &val)
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
		m_in.get();
		RpcValue key_val = parseAtPos();
		map[key] = key_val;
		ch = skipGarbage();
		if (ch == ',')
			ch = nextValidChar();
		//PARSE_EXCEPTION("unexpected delimiter in IMap, got " + dump_char(ch));
	}
	m_in.get();
	val = map;
	return true;
}

bool CponReader::parseIMap(RpcValue &val)
{
	if (!getString(S_IMAP_BEGIN))
		return false;
	skipGarbage();
	RpcValue::IMap imap = parseIMapContent('}');
	val = imap;
	return true;
}

bool CponReader::parseArray(RpcValue &ret_val)
{
	if (!getString(S_ARRAY_BEGIN))
		return false;
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
	m_in.get();
	ret_val = arr;
	return true;
}

bool CponReader::parseDateTime(RpcValue &val)
{
	if(!getString(S_DATETIME_BEGIN))
		return false;
	m_in.unget();
	std::string s;
	if(parseStringHelper(s)) {
		val = RpcValue::DateTime::fromUtcString(s);
		return true;
	}
	PARSE_EXCEPTION("error parsing DateTime");
}

uint64_t CponReader::parseDecimalUnsigned(int radix)
{
	uint64_t ret = 0;
	while(!m_in.eof()) {
		auto ch = m_in.peek();
		if(in_range(ch, '0', '9')) {
			ret *= radix;
			ret += (ch - '0');
			m_in.get();
		}
		else {
			break;
		}
	}
	return ret;
}

RpcValue::IMap CponReader::parseIMapContent(char closing_bracket)
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
		m_in.get();
		RpcValue val = parseAtPos();
		map[key] = val;
		ch = skipGarbage();
		if (ch == ',')
			ch = nextValidChar();
	}
	m_in.get();
	return map;
}

RpcValue::MetaData CponReader::parseMetaDataContent(char closing_bracket)
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
	m_in.get();
	return RpcValue::MetaData(std::move(imap), std::move(smap));
}

bool CponReader::parseStringHelper(std::string &val)
{
	char ch = currentChar();
	if (ch != '"')
		return false;

	std::string str_val;
	long last_escaped_codepoint = -1;
	while (true) {
		if (m_in.eof())
			PARSE_EXCEPTION("unexpected end of input in string");

		ch = m_in.get();

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
		if (m_in.eof())
			PARSE_EXCEPTION("unexpected end of input in string");

		ch = m_in.get();

		if (ch == 'u') {
			// Extract 4-byte escape sequence
			std::string esc;
			for (int i = 0; i < 4 && !m_in.eof(); ++i)
				esc += m_in.get();
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
			continue;
		}

		encodeUtf8(last_escaped_codepoint, str_val);
		last_escaped_codepoint = -1;

		if (ch == 'b') {
			str_val += '\b';
		}
		else if (ch == 'f') {
			str_val += '\f';
		}
		else if (ch == 'n') {
			str_val += '\n';
		}
		else if (ch == 'r') {
			str_val += '\r';
		}
		else if (ch == 't') {
			str_val += '\t';
		}
		else if (ch == '"' || ch == '\\' || ch == '/') {
			str_val += ch;
		}
		else {
			PARSE_EXCEPTION("invalid escape character " + dump_char(ch));
		}
	}
}

} // namespace chainpack
} // namespace shv
