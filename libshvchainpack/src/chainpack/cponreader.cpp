#include "cponreader.h"

#include <iostream>
#include <cmath>

namespace shv {
namespace chainpack {

enum {exception_aborts = 0};

#define PARSE_EXCEPTION(msg) {\
	std::clog << __FILE__ << ':' << __LINE__; \
	std::clog << ' ' << (msg) << std::endl; \
	if(exception_aborts) { \
		abort(); \
	} \
	else { \
		throw CponReader::ParseException(msg); \
	} \
}

#define TOKENIZER_EXCEPTION(msg) {\
	std::clog << __FILE__ << ':' << __LINE__; \
	char buff[40]; \
	m_in.readsome(buff, sizeof(buff)); \
	std::clog << ' ' << (msg) << " at pos: " << m_in.tellg() << " near to: " << buff << std::endl; \
	ctx.token = AbstractStreamReader::Token::Error; \
	ctx.value = msg; \
	return false; \
}



CponReader &CponReader::operator >>(RpcValue &value)
{
	readValue(value);
	return *this;
}

CponReader &CponReader::operator >>(RpcValue::MetaData &meta_data)
{
	readMetaData(meta_data);
	return *this;
}

void CponReader::readMetaData(RpcValue::MetaData &meta_data)
{
	AbstractStreamTokenizer::TokenType token_type = m_tokenizer.nextToken();
	if(token_type == CponTokenizer::TokenType::MetaDataBegin) {
		while(true) {
			RpcValue key;
			switch(m_tokenizer.nextToken()) {
			case(CponTokenizer::TokenType::MetaDataEnd):
				break;
			case(CponTokenizer::TokenType::Key):
				key = m_tokenizer.value();
				break;
			case(CponTokenizer::TokenType::Value):
				if(key.isString())
					meta_data.setValue(key.toString(), m_tokenizer.value());
				else
					meta_data.setValue(key.toUInt(), m_tokenizer.value());
				break;
			default:
				PARSE_EXCEPTION("Unexpected tokenizer return value.");
				return;
			}
		}
	}
	else {
		m_tokenizer.unget();
	}
}

void CponReader::readValue(RpcValue &val)
{
	RpcValue::MetaData md;
	readMetaData(md);
	AbstractStreamTokenizer::TokenType token_type = m_tokenizer.nextToken();
	switch(token_type) {
	case(CponTokenizer::TokenType::MapBegin): {
		RpcValue::Map map;
		while(true) {
			std::string key;
			switch(m_tokenizer.nextToken()) {
			case(CponTokenizer::TokenType::MapEnd):
				val = map;
				return;
			case(CponTokenizer::TokenType::Key):
				key = m_tokenizer.value().toString();
				break;
			case(CponTokenizer::TokenType::Value):
				map[key] = m_tokenizer.value();
				break;
			default:
				PARSE_EXCEPTION("Unexpected tokenizer return value.");
				return;
			}
		}
		break;
	}
	}
	if(!md.isEmpty())
		val.setMetaData(std::move(md));
}
#if 0
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
		getValue(val);
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
		getValue(val);
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
		getValue(val);
		map[key] = val;
	}
	val = map;
}

void CponReader::parseMetaData(RpcValue::MetaData &meta_data)
{
	RpcValue::IMap imap;
	RpcValue::Map smap;
	while (true) {
		char ch = getValidChar();
		if (ch == ',')
			continue;
		if(ch == '>')
			break;
		m_in.unget();
		RpcValue key;
		getValue(key);
		if(!(key.type() == RpcValue::Type::Int || key.type() == RpcValue::Type::UInt)
		   && !(key.type() == RpcValue::Type::String))
			PARSE_EXCEPTION("key expected");
		ch = getValidChar();
		if (ch != ':')
			PARSE_EXCEPTION("expected ':' in MetaData, got " + dump_char(ch));
		RpcValue val;
		getValue(val);
		if(key.type() == RpcValue::Type::String)
			smap[key.toString()] = val;
		else
			imap[key.toUInt()] = val;
	}
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
		getValue(val);
		if(arr.empty()) {
			arr = RpcValue::Array(val.type());
		}
		else {
			if(val.type() != arr.type())
				PARSE_EXCEPTION("Mixed types in Array");
		}
		arr.push_back(RpcValue::Array::makeElement(val));
	}
	ret_val = arr;
}
#endif

} // namespace chainpack
} // namespace shv
