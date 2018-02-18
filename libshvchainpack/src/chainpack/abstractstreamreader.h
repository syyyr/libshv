#pragma once

#include "rpcvalue.h"

#include "../shvchainpackglobal.h"

#include <istream>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT AbstractStreamReader
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
	AbstractStreamReader(std::istream &in);
	virtual ~AbstractStreamReader() {}

	RpcValue read();

	virtual void read(RpcValue::MetaData &meta_data) = 0;
	virtual void read(RpcValue &val) = 0;
protected:
	std::istream &m_in;
};

} // namespace chainpack
} // namespace shv

