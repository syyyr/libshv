#include "cpon.h"
#include "cponreader.h"

#include <iostream>
#include <cmath>

namespace shv {
namespace chainpack {

#define PARSE_EXCEPTION(msg) {\
	char buff[40]; \
	m_in.readsome(buff, sizeof(buff)); \
	if(exception_aborts) { \
		std::clog << __FILE__ << ':' << __LINE__;  \
		std::clog << ' ' << (msg) << " at pos: " << m_in.tellg() << " near to: " << buff << std::endl; \
		abort(); \
	} \
	else { \
		throw CponReader::ParseException(msg + std::string(" at pos: ") + std::to_string(m_in.tellg()) + " near to: " + buff); \
	} \
}

namespace {
enum {exception_aborts = 0};

const std::string S_NULL("null");
const std::string S_TRUE("true");
const std::string S_FALSE("false");

const char S_IMAP_BEGIN('i');
const char S_ARRAY_BEGIN('a');
//const char S_ESC_BLOB_BEGIN('b');
//const char S_HEX_BLOB_BEGIN('x');
const char S_DATETIME_BEGIN('d');
//const char S_LIST_BEGIN('[');
//const char S_LIST_END(']');
//const char S_MAP_BEGIN('{');
//const char S_MAP_END('}');
//const char S_META_BEGIN('<');
//const char S_META_END('>');
const char S_DECIMAL_END('n');
const char S_UNSIGNED_END('u');

const int MAX_RECURSION_DEPTH = 1000;

class DepthScope
{
public:
	DepthScope(int &depth) : m_depth(depth) {m_depth++;}
	~DepthScope() {m_depth--;}
private:
	int &m_depth;
};

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

}

CponReader &CponReader::operator >>(RpcValue &value)
{
	read(value);
	return *this;
}

CponReader &CponReader::operator >>(RpcValue::MetaData &meta_data)
{
	auto ch = getValidChar();
	if(ch == '<')
		read(meta_data);
	return *this;
}

int CponReader::getChar()
{
	auto ch = m_in.get();
	if(m_in.eof())
		PARSE_EXCEPTION("Unexpected end of stream.");
	return ch;
}

void CponReader::read(RpcValue &val, std::string &err)
{
	err.clear();
	try {
		read(val);
	}
	catch (ParseException &e) {
		err = e.what();
	}
}

void CponReader::read(RpcValue &val)
{
	if (m_depth > MAX_RECURSION_DEPTH)
		PARSE_EXCEPTION("maximum nesting depth exceeded");
	DepthScope{m_depth};

	RpcValue::Type type = RpcValue::Type::Invalid;
	RpcValue::MetaData md;
	//bool hex_blob = false;
	auto ch = getValidChar();
	if(ch == '<') {
		m_in.unget();
		read(md);
		ch = getValidChar();
	}
	switch (ch) {
	case S_IMAP_BEGIN: {
		ch = getChar();
		if(ch == '{')
			type = RpcValue::Type::IMap;
		else
			PARSE_EXCEPTION("Invalid IMap prefix.");
		break;
	}
	case S_ARRAY_BEGIN: {
		ch = m_in.peek();
		if(ch >= '0' && ch <= '9') {
			int i;
			parseInteger(i);
			ch = getChar();
		}
		else {
			ch = getChar();
		}
		if(ch == '[')
			type = RpcValue::Type::Array;
		else
			PARSE_EXCEPTION("Invalid Array prefix.");
		break;
	}
	/*
	case S_ESC_BLOB_BEGIN: {
		hex_blob = false;
		ch = getChar();
		if(ch == '"')
			type = RpcValue::Type::Blob;
		else
			PARSE_EXCEPTION("Invalid Blob prefix.");
		break;
	}
	case S_HEX_BLOB_BEGIN: {
		hex_blob = true;
		ch = getChar();
		if(ch == '"')
			type = RpcValue::Type::Blob;
		else
			PARSE_EXCEPTION("Invalid Blob prefix.");
		break;
	}
	*/
	case S_DATETIME_BEGIN: {
		ch = getChar();
		if(ch == '"')
			type = RpcValue::Type::DateTime;
		else
			PARSE_EXCEPTION("Invalid DateTime prefix.");
		break;
	}
	case '{':  type = RpcValue::Type::Map; break;
	case '[':  type = RpcValue::Type::List; break;
	case '"':  type = RpcValue::Type::String; break;
	case 'n':
		m_in.unget();
		type = RpcValue::Type::Null;
		break;
	case 'f':
	case 't':
		m_in.unget();
		type = RpcValue::Type::Bool;
		break;
	default:
		if((ch >= '0' && ch <= '9') || (ch == '-') || (ch == '.')) {
			m_in.unget();
			type = RpcValue::Type::Int;
		}
		else
			PARSE_EXCEPTION("Invalid input.");
		break;
	}

	switch (type) {
	case RpcValue::Type::List: parseList(val); break;
	case RpcValue::Type::Array: parseArray(val); break;
	case RpcValue::Type::Map: parseMap(val); break;
	case RpcValue::Type::IMap: parseIMap(val); break;
	case RpcValue::Type::Null: parseNull(val); break;
	case RpcValue::Type::Bool: parseBool(val); break;
	//case RpcValue::Type::Blob: parseBlob(val, hex_blob); break;
	case RpcValue::Type::String: parseString(val); break;
	case RpcValue::Type::DateTime: parseDateTime(val); break;
	case RpcValue::Type::Int:
	case RpcValue::Type::UInt:
	case RpcValue::Type::Double:
	case RpcValue::Type::Decimal: parseNumber(val); break;
	case RpcValue::Type::Invalid:
		PARSE_EXCEPTION("Invalid type.");
		break;
	}
	if(!md.isEmpty())
		val.setMetaData(std::move(md));
}

char CponReader::getValidChar()
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

std::string CponReader::getString(size_t n)
{
	std::string ret;
	for (size_t i = 0; i < n; ++i)
		ret += getChar();
	return ret;
}

void CponReader::decodeUtf8(long pt, std::string &out)
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

void CponReader::parseNull(RpcValue &val)
{
	std::string s = getString(S_NULL.size());
	if(s != S_NULL)
		PARSE_EXCEPTION("Parse null error, got: " + s);
	val = RpcValue(nullptr);
}

void CponReader::parseBool(RpcValue &val)
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

void CponReader::parseString(RpcValue &val)
{
	std::string s;
	parseStringHelper(s);
	val = s;
}
/*
void CponReader::parseBlob(RpcValue &val, bool hex_blob)
{
	std::string s;
	if(hex_blob) {
		parseStringHelper(s);
		s = Utils::fromHex(s);
	}
	else {
		parseCStringHelper(s);
	}
	val = RpcValue::Blob{s};
}
*/
void CponReader::parseNumber(RpcValue &val)
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
		val = RpcValue(int_part * sign);
		break;
	case RpcValue::Type::UInt:
		val = RpcValue((uint64_t)int_part);
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

void CponReader::parseList(RpcValue &val)
{
	RpcValue::List lst;
	while (true) {
		auto ch = getValidChar();
		if (ch == ',')
			continue;
		if (ch == ']')
			break;
		m_in.unget();
		RpcValue val;
		read(val);
		lst.push_back(val);
	}
	val = lst;
}

void CponReader::parseMap(RpcValue &val)
{
	RpcValue::Map map;
	while (true) {
		auto ch = getValidChar();
		if (ch == ',')
			continue;
		if (ch == '}')
			break;
		if(ch != '"')
			PARSE_EXCEPTION("expected '\"' in map key, got " + dump_char(ch));
 		std::string key;
		parseStringHelper(key);
		ch = getValidChar();
		if (ch != ':')
			PARSE_EXCEPTION("expected ':' in Map, got " + dump_char(ch));
		RpcValue val;
		read(val);
		map[key] = val;
	}
	val = map;
}

void CponReader::parseIMap(RpcValue &val)
{
	RpcValue::IMap map;
	while (true) {
		auto ch = getValidChar();
		if (ch == ',')
			continue;
		if(ch == '}')
			break;
		m_in.unget();
		RpcValue val;
		parseNumber(val);
		if(!(val.type() == RpcValue::Type::Int || val.type() == RpcValue::Type::UInt))
			PARSE_EXCEPTION("int key expected");
		RpcValue::UInt key = val.toUInt();
		ch = getValidChar();
		if (ch != ':')
			PARSE_EXCEPTION("expected ':' in IMap, got " + dump_char(ch));
		read(val);
		map[key] = val;
	}
	val = map;
}

void CponReader::read(RpcValue::MetaData &meta_data)
{
	RpcValue::IMap imap;
	RpcValue::Map smap;
	char ch = getValidChar();
	if(ch == Cpon::C_META_BEGIN) {
		while (true) {
			ch = getValidChar();
			if (ch == ',')
				continue;
			if(ch == '>')
				break;
			m_in.unget();
			RpcValue key;
			read(key);
			if(!(key.type() == RpcValue::Type::Int || key.type() == RpcValue::Type::UInt)
			   && !(key.type() == RpcValue::Type::String))
				PARSE_EXCEPTION("key expected");
			ch = getValidChar();
			if (ch != ':')
				PARSE_EXCEPTION("expected ':' in MetaData, got " + dump_char(ch));
			RpcValue val;
			read(val);
			if(key.type() == RpcValue::Type::String)
				smap[key.toString()] = val;
			else
				imap[key.toUInt()] = val;
		}
		ch = getValidChar();
	}
	m_in.unget();
	meta_data = RpcValue::MetaData(std::move(imap), std::move(smap));
}

void CponReader::parseArray(RpcValue &ret_val)
{
	RpcValue::Array arr;
	while (true) {
		auto ch = getValidChar();
		if (ch == ',')
			continue;
		if (ch == ']')
			break;
		m_in.unget();
		RpcValue val;
		read(val);
		if(arr.empty()) {
			arr = RpcValue::Array(val.type());
		}
		else {
			if(val.type() != arr.type())
				PARSE_EXCEPTION("Mixed types in Array: " + val.toCpon());
		}
		arr.push_back(RpcValue::Array::makeElement(val));
	}
	ret_val = arr;
}

void CponReader::parseDateTime(RpcValue &val)
{
	std::string s;
	parseStringHelper(s);
	val = RpcValue::DateTime::fromUtcString(s);
}

uint64_t CponReader::parseInteger(int &cnt)
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

void CponReader::parseStringHelper(std::string &val)
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

void CponReader::parseCStringHelper(std::string &val)
{
	std::string str_val;
	while (true) {
		if (m_in.eof())
			PARSE_EXCEPTION("unexpected end of input in string");

		auto ch = m_in.get();
		//std::cout << "A -> " << ch << std::endl;
		if (ch == '"') {
			val = str_val;
			return;
		}

		if (in_range(ch, 0, 0x1f))
			PARSE_EXCEPTION("unescaped " + dump_char(ch) + " in string");

		// The usual case: non-escaped characters
		if (ch != '\\') {
			str_val += ch;
			continue;
		}

		// Handle escapes
		if (m_in.eof())
			PARSE_EXCEPTION("unexpected end of input in string");

		ch = m_in.get();
		//std::cout << "B -> " << ch << std::endl;
		/*
		if (ch == 'x') {
			// Extract 2-byte escape sequence
			std::string esc;
			for (int i = 0; i < 2 && !m_in.eof(); ++i)
				esc += (char)m_in.get();
			if (esc.length() < 2)
				PARSE_EXCEPTION("bad \\x escape: " + esc);

			ch = 0;
			for (size_t j = 0; j < 2; j++) {
				char c = Utils::fromHex(esc[j]);
				if (c < 0)
					PARSE_EXCEPTION("bad \\x escape: " + esc);
				ch = 16*ch + c;
			}
			str_val += ch;
			continue;
		}
		*/
		switch (ch) {
		case '\\': str_val += '\\'; break;
		case '"' : str_val += '"'; break;
		case 'b': str_val += '\b'; break;
		case 'f': str_val += '\f'; break;
		case 'n': str_val += '\n'; break;
		case 'r': str_val += '\r'; break;
		case 't': str_val += '\t'; break;
		case '0': str_val += '\0'; break;
		default:
			PARSE_EXCEPTION("invalid escape character " + dump_char(ch));
		}
	}
}

} // namespace chainpack
} // namespace shv
