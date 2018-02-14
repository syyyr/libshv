#include "cpontokenizer.h"

#include <iostream>
#include <cmath>

namespace shv {
namespace chainpack {

#define LOG_ERROR(msg) {\
	std::clog << __FILE__ << ':' << __LINE__; \
	char buff[40]; \
	m_in.readsome(buff, sizeof(buff)); \
	std::clog << ' ' << (msg) << " at pos: " << m_in.tellg() << " near to: " << buff << std::endl; \
}

#define PARSE_EXCEPTION(msg) {\
	LOG_ERROR(msg) \
	if(exception_aborts) { \
		abort(); \
	} \
	else { \
		throw AbstractStreamTokenizer::ParseException(msg); \
	} \
}

namespace {
enum {exception_aborts = 0};

const std::string S_NULL("null");
const std::string S_TRUE("true");
const std::string S_FALSE("false");

const char S_CONTAINER_DELIMITER(',');
const char S_IMAP_BEGIN('i');
const char S_ARRAY_BEGIN('a');
const char S_BLOB_BEGIN('x');
const char S_DATETIME_BEGIN('d');
const char S_LIST_BEGIN('[');
const char S_LIST_END(']');
const char S_MAP_BEGIN('{');
const char S_MAP_END('}');
const char S_META_BEGIN('<');
const char S_META_END('>');
const char S_DECIMAL_END('n');
const char S_UNSIGNED_END('u');

//const int MAX_RECURSION_DEPTH = 1000;
/*
class DepthScope
{
public:
	DepthScope(int &depth) : m_depth(depth) {m_depth++;}
	~DepthScope() {m_depth--;}
private:
	int &m_depth;
};
*/
inline bool in_range(long x, long lower, long upper)
{
	return (x >= lower && x <= upper);
}

inline std::string dump_char(char c)
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

void decodeUtf8(long pt, std::string &out)
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

}

int CponTokenizer::getChar()
{
	int ch = m_in.get();
	if(ch < 0)
		PARSE_EXCEPTION("Unexpected end of stream.");
	return ch;
}

char CponTokenizer::getValidChar()
{
	while(true) {
		auto ch = getChar();
		if (ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t') {
		}
		else if(ch == '/') {
			ch = getChar();
			if(ch == '/') {
				// to end of line comment
				while (!m_in.eof()) {
					ch = m_in.get();
					if(ch == '\n')
						break;
				}
			}
			else if(ch == '*') {
				// multi line comment
				char prev_ch = -1;
				ch = getChar();
				while (!m_in.eof()) {
					ch = m_in.get();
					if(prev_ch == '*' && ch == '/')
						break;
					prev_ch = ch;
				}
				if(!(prev_ch == '*' && ch == '/'))
					PARSE_EXCEPTION("Unclosed multiline comment.");
			}
			else {
				PARSE_EXCEPTION("Invalid comment.");
			}
		}
		else {
			return ch;
		}
	}
}

std::string CponTokenizer::getString(size_t n)
{
	std::string ret;
	for (size_t i = 0; i < n; ++i)
		ret += getChar();
	return ret;
}

void CponTokenizer::parseNull(RpcValue &val)
{
	std::string s = getString(S_NULL.size());
	if(s != S_NULL)
		PARSE_EXCEPTION("Parse null error, got: " + s);
	val = RpcValue(nullptr);
}

void CponTokenizer::parseBool(RpcValue &val)
{
	std::string s = getString(S_TRUE.size());
	if(s == S_TRUE) {
		val = RpcValue(true);
		return;
	}
	else {
		char ch = getChar();
		s += ch;
		if(s == S_FALSE) {
			val = RpcValue(false);
			return;
		}
	}
	PARSE_EXCEPTION("Parse bool error, got: " + s);
}

void CponTokenizer::parseString(RpcValue &val)
{
	std::string s;
	parseStringHelper(s);
	val = s;
}

void CponTokenizer::parseBlob(RpcValue &val)
{
	std::string s;
	parseStringHelper(s);
	s = Utils::fromHex(s);
	val = RpcValue::Blob{s};
}

void CponTokenizer::parseNumber(RpcValue &val)
{
	int sign = 1;
	int64_t int_part = 0;
	int64_t dec_part = 0;
	int dec_cnt = 0;
	int exponent = std::numeric_limits<int>::max();
	RpcValue::Type type = RpcValue::Type::Invalid;

	char ch = m_in.peek();
	if(ch != '.') {
		type = RpcValue::Type::Int;
		int cnt;
		int_part = parseInteger(cnt);
		if (cnt == 0)
			PARSE_EXCEPTION("number integer part missing");
		if (m_in.peek() == S_UNSIGNED_END) {
			type = RpcValue::Type::UInt;
			m_in.get();
		}
		//m_in.unget();
	}
	if (m_in.peek() == '.') {
		m_in.get();
		dec_part = parseInteger(dec_cnt);
		type = RpcValue::Type::Double;
		if (m_in.peek() == S_DECIMAL_END) {
			type = RpcValue::Type::Decimal;
			m_in.get();
		}
	}
	else if(m_in.peek() == 'e' || m_in.peek() == 'E') {
		m_in.get();
		int cnt;
		exponent = parseInteger(cnt);
		if (cnt == 0)
			PARSE_EXCEPTION("double exponent part missing");
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
}

void CponTokenizer::parseDateTime(RpcValue &val)
{
	std::string s;
	parseStringHelper(s);
	val = RpcValue::DateTime::fromUtcString(s);
}

uint64_t CponTokenizer::parseInteger(int &cnt)
{
	int64_t ret = 0;
	cnt = 0;
	int sig = 0;
	int radix = 10;
	while(true) {
		char ch = m_in.get();
		if(ch == 'x' && cnt == 1 && ret == 0) {
			radix = 16;
		}
		else if(ch == '-' && sig == 0) {
			sig = -1;
		}
		else if(ch == '+' && sig == 0) {
			sig = 1;
		}
		else if(in_range(ch, '0', '9')) {
			ret *= radix;
			ret += (ch - '0');
			cnt++;
		}
		else if(radix == 16 && in_range(ch, 'a', 'f')) {
			ret *= radix;
			ret += (ch - 'a' + 10);
			cnt++;
		}
		else if(radix == 16 && in_range(ch, 'A', 'F')) {
			ret *= radix;
			ret += (ch - 'A' + 10);
			cnt++;
		}
		else {
			m_in.unget();
			break;
		}
	}
	if(sig == 0)
		sig = 1;
	ret *= sig;
	return ret;
}

void CponTokenizer::parseStringHelper(std::string &val)
{
	std::string str_val;
	long last_escaped_codepoint = -1;
	while (true) {
		if (m_in.eof())
			PARSE_EXCEPTION("unexpected end of input in string");

		auto ch = m_in.get();

		if (ch == '"') {
			decodeUtf8(last_escaped_codepoint, str_val);
			val = str_val;
			return;
		}

		if (in_range(ch, 0, 0x1f))
			PARSE_EXCEPTION("unescaped " + dump_char(ch) + " in string");

		// The usual case: non-escaped characters
		if (ch != '\\') {
			decodeUtf8(last_escaped_codepoint, str_val);
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
				decodeUtf8((((last_escaped_codepoint - 0xD800) << 10)
							 | (codepoint - 0xDC00)) + 0x10000, str_val);
				last_escaped_codepoint = -1;
			} else {
				decodeUtf8(last_escaped_codepoint, str_val);
				last_escaped_codepoint = codepoint;
			}
			continue;
		}

		decodeUtf8(last_escaped_codepoint, str_val);
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

AbstractStreamTokenizer::TokenType CponTokenizer::nextToken()
{
	if(isWithinMap()) {
		if(m_context.state == CponTokenizer::State::KeyExpected) {
			auto ch = getValidChar();
			if(ch == S_CONTAINER_DELIMITER)
				ch = getValidChar();
			if(ch == S_MAP_END || ch == S_META_END) {
				popContext();
				switch (m_context.containerType) {
				case AbstractStreamTokenizer::ContainerType::IMap:
					return CponTokenizer::TokenType::IMapEnd;
				case AbstractStreamTokenizer::ContainerType::Map:
					return CponTokenizer::TokenType::MapEnd;
				case AbstractStreamTokenizer::ContainerType::MetaData:
					return CponTokenizer::TokenType::MetaDataEnd;
				default:
					PARSE_EXCEPTION("Unexpected '}'");
					return CponTokenizer::TokenType::Invalid;
				}
			}

			switch (m_context.containerType) {
			case AbstractStreamTokenizer::ContainerType::IMap:
				parseNumber(m_context.value);
				break;
			case AbstractStreamTokenizer::ContainerType::Map:
				parseString(m_context.value);
				break;
			case AbstractStreamTokenizer::ContainerType::MetaData:
				m_context.state = CponTokenizer::State::ValueExpected;
				nextToken();
				if(!m_context.value.isString() && !m_context.value.isUInt() && !m_context.value.isInt()) {
					PARSE_EXCEPTION("Invalid meta data key.");
					return CponTokenizer::TokenType::Invalid;
				}
				break;
			default:
				PARSE_EXCEPTION("Key enexpected");
				return CponTokenizer::TokenType::Invalid;
			}
			ch = getValidChar();
			if(ch == ':') {
				m_context.state = CponTokenizer::State::ValueExpected;
				return CponTokenizer::TokenType::Key;
			}
			PARSE_EXCEPTION("Invalid key-val delimiter.");
			return CponTokenizer::TokenType::Invalid;
		}
	}

	RpcValue::Type value_type = RpcValue::Type::Invalid;
	auto ch = getValidChar();

	// skip container delimiter, if any
	if(ch == S_CONTAINER_DELIMITER)
		ch = getValidChar();

	switch (ch) {
	case S_LIST_END:  {
		popContext();
		return (m_context.containerType == ContainerType::Array)? TokenType::ArrayEnd: TokenType::ListEnd;
	}
	case S_LIST_BEGIN:  {
		pushContext();
		return TokenType::ListBegin;
	}
	case S_ARRAY_BEGIN: {
		ch = getChar();
		if(ch == S_LIST_BEGIN) {
			pushContext();
			return TokenType::ArrayBegin;
		}
		else
			PARSE_EXCEPTION("Invalid Array prefix.");
		break;
	}
	case S_MAP_BEGIN: {
		pushContext();
		m_context.state = CponTokenizer::State::KeyExpected;
		m_context.containerType = CponTokenizer::ContainerType::Map;
		return TokenType::MapBegin;
	}
	case S_META_BEGIN: {
		pushContext();
		m_context.state = CponTokenizer::State::KeyExpected;
		m_context.containerType = CponTokenizer::ContainerType::MetaData;
		return TokenType::MetaDataBegin;
	}
	case S_IMAP_BEGIN: {
		ch = getChar();
		if(ch == S_MAP_BEGIN) {
			pushContext();
			m_context.state = CponTokenizer::State::KeyExpected;
			m_context.containerType = CponTokenizer::ContainerType::IMap;
			return TokenType::IMapBegin;
		}
		else
			PARSE_EXCEPTION("Invalid IMap prefix.");
		break;
	}
	case S_BLOB_BEGIN: {
		ch = getChar();
		if(ch == '"')
			value_type = RpcValue::Type::Blob;
		else
			PARSE_EXCEPTION("Invalid Blob prefix.");
		break;
		m_in.unget();
	}
	case S_DATETIME_BEGIN: {
		ch = getChar();
		if(ch == '"')
			value_type = RpcValue::Type::DateTime;
		else
			PARSE_EXCEPTION("Invalid DateTime prefix.");
		break;
		m_in.unget();
	}
	case '"':
		m_in.unget();
		value_type = RpcValue::Type::String;
		break;
	case 'n':
		m_in.unget();
		value_type = RpcValue::Type::Null;
		break;
	case 'f':
	case 't':
		m_in.unget();
		value_type = RpcValue::Type::Bool;
		break;
	default:
		if((ch >= '0' && ch <= '9') || (ch == '-') || (ch == '.')) {
			m_in.unget();
			value_type = RpcValue::Type::Int;
		}
		else
			PARSE_EXCEPTION("Invalid input.");
		break;
	}

	switch (value_type) {
	case RpcValue::Type::Null: parseNull(m_context.value); break;
	case RpcValue::Type::Bool: parseBool(m_context.value); break;
	case RpcValue::Type::Blob: parseBlob(m_context.value); break;
	case RpcValue::Type::String: parseString(m_context.value); break;
	case RpcValue::Type::DateTime: parseDateTime(m_context.value); break;
	case RpcValue::Type::Int:
	case RpcValue::Type::UInt:
	case RpcValue::Type::Double:
	case RpcValue::Type::Decimal: parseNumber(m_context.value); break;
	case RpcValue::Type::Invalid:
		PARSE_EXCEPTION("Invalid type.");
		return CponTokenizer::TokenType::Invalid;
	}

	if(isWithinMap()) {
		m_context.state = CponTokenizer::State::KeyExpected;
	}

	return CponTokenizer::TokenType::Value;
}

} // namespace chainpack
} // namespace shv
