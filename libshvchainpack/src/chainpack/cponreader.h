#ifndef CPONREADER_H
#define CPONREADER_H

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

class SHVCHAINPACK_DECL_EXPORT CponReader
{
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
	CponReader(std::istream &in) : m_in(in) {}

	CponReader& operator >>(RpcValue &value);
	CponReader& operator >>(RpcValue::MetaData &meta_data);
private:
	RpcValue parseAtPos();

	uint64_t parseDecimalUnsigned(int radix);
	RpcValue::IMap parseIMapContent(char closing_bracket);
	RpcValue::MetaData parseMetaDataContent(char closing_bracket);
	bool parseStringHelper(std::string &val);

	bool parseMetaData(RpcValue::MetaData &meta_data);

	bool parseNull(RpcValue &val);
	bool parseBool(RpcValue &val);
	bool parseString(RpcValue &val);
	bool parseBlob(RpcValue &val);
	bool parseNumber(RpcValue &val);
	bool parseList(RpcValue &val);
	bool parseArray(RpcValue &ret_val);
	bool parseMap(RpcValue &val);
	bool parseIMap(RpcValue &val);
	bool parseDateTime(RpcValue &val);

	void skipWhiteSpace();
	bool skipComment();

	char skipGarbage();
	char nextValidChar();
	char currentChar();

	void encodeUtf8(long pt, std::string &out);

	bool getString(const char *str);
private:
	std::istream &m_in;
	int m_depth = 0;
};

} // namespace chainpack
} // namespace shv

#endif // CPONREADER_H
