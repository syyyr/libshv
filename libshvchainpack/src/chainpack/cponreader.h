#pragma once

#include "abstractstreamreader.h"
#include "cpontokenizer.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT CponReaderOptions
{
	/*
	bool m_translateIds = false;
	bool m_indent = 0;
public:
	//CponReaderOptions() {}

	bool setTranslateIds() const {return m_translateIds;}
	CponReaderOptions& setTranslateIds(bool b) {m_translateIds = b; return *this;}

	bool isIndent() const {return m_indent;}
	CponReaderOptions& setIndent(bool i) {m_indent = i; return *this;}
	*/
};

class SHVCHAINPACK_DECL_EXPORT CponReader : public AbstractStreamReader
{
	using Super = AbstractStreamReader;
public:
	class SHVCHAINPACK_DECL_EXPORT ParseException : public std::exception
	{
		using Super = std::exception;
	public:
		ParseException(std::string &&message) : m_message(std::move(message)) {}
		const std::string& mesage() const {return m_message;}
		const char *what() const noexcept override {return m_message.data();}
	private:
		std::string m_message;
	};
public:
	CponReader(std::istream &in) : Super(), m_tokenizer(in) {}

	CponReader& operator >>(RpcValue &value);
	CponReader& operator >>(RpcValue::MetaData &meta_data);
private:
	void readValue(RpcValue &val);
	void readMetaData(RpcValue::MetaData &meta_data);
	//int getChar();
private:
	//RpcValue parseAtPos();
	/*
	uint64_t parseInteger(int &cnt);
	RpcValue::IMap parseIMapContent(char closing_bracket);
	RpcValue::MetaData parseMetaDataContent(char closing_bracket);
	void parseStringHelper(std::string &val);

	void parseMetaData(RpcValue::MetaData &meta_data);

	void parseNull(RpcValue &val);
	void parseBool(RpcValue &val);
	void parseString(RpcValue &val);
	void parseBlob(RpcValue &val);
	void parseNumber(RpcValue &val);
	void parseList(RpcValue &val);
	void parseArray(RpcValue &ret_val);
	void parseMap(RpcValue &val);
	void parseIMap(RpcValue &val);
	void parseDateTime(RpcValue &val);

	char getValidChar();
	std::string getString(size_t n);

	void decodeUtf8(long pt, std::string &out);
	*/
private:
	CponTokenizer m_tokenizer;
};

} // namespace chainpack
} // namespace shv

