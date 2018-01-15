#pragma once

#include "rpcvalue.h"

namespace shv {
namespace core {
namespace chainpack {

class SHVCORE_DECL_EXPORT CponProtocol final
{
public:
	class SHVCORE_DECL_EXPORT ParseException : public std::exception
	{
		using Super = std::exception;
	public:
		ParseException(std::string &&message, int pos) : m_message(std::move(message)), m_pos(pos) {}
		const std::string& mesage() const {return m_message;}
		int pos() const {return m_pos;}
		const char *what() const noexcept override {return m_message.data();}
	private:
		std::string m_message;
		int m_pos;
	};
public:
	static RpcValue parse(const std::string & in);
	static std::string serialize(const RpcValue &value);
	static void serialize(std::ostream &out, const RpcValue &value);
private:
	CponProtocol(const std::string &str);
	RpcValue parseAtPos();

	void skipWhiteSpace();
	bool skipComment();

	char skipGarbage();
	char nextValidChar();
	char currentChar();

	uint64_t parseDecimalUnsigned(int radix);
	RpcValue::IMap parseIMapContent();
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
private:
	void encodeUtf8(long pt, std::string & out);
public:
	static void serialize(std::nullptr_t, std::ostream &out);
	static void serialize(double value, std::ostream &out);
	static void serialize(RpcValue::Int value, std::ostream &out);
	static void serialize(RpcValue::UInt value, std::ostream &out);
	static void serialize(bool value, std::ostream &out);
	static void serialize(RpcValue::DateTime value, std::ostream &out);
	static void serialize(RpcValue::Decimal value, std::ostream &out);
	static void serialize(const std::string &value, std::ostream &out);
	static void serialize(const RpcValue::Blob &value, std::ostream &out);
	static void serialize(const RpcValue::List &values, std::ostream &out);
	static void serialize(const RpcValue::Array &values, std::ostream &out);
	static void serialize(const RpcValue::Map &values, std::ostream &out);
	static void serialize(const RpcValue::IMap &values, std::ostream &out);
	static void serialize(const RpcValue::MetaData &value, std::ostream &out);
private:
	const std::string &m_str;
	//std::string &m_err;
	size_t m_pos = 0;
	int m_depth = 0;
};

} // namespace chainpack
}
} // namespace shv
