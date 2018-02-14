#pragma once

#include "rpcvalue.h"

#include <istream>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT AbstractStreamTokenizer
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
	enum class TokenType
	{
		Invalid,
		Error,
		Key,
		Value,
		ArrayBegin,
		ListBegin,
		MapBegin,
		IMapBegin,
		MetaDataBegin,
		ArrayEnd,
		ListEnd,
		MapEnd,
		IMapEnd,
		MetaDataEnd,
	};

	enum class State
	{
		KeyExpected,
		ValueExpected,
	};

	enum class ContainerType
	{
		NotAContainer,
		Map,
		IMap,
		MetaData,
		List,
		Array,
	};

	struct Context
	{
		//friend AbstractStreamReader;
		//TokenType tokenType = TokenType::Invalid;
		RpcValue value;
		State state = State::ValueExpected;
		ContainerType containerType = ContainerType::NotAContainer;
	};

	AbstractStreamTokenizer(std::istream &in);

	const RpcValue& value() const {return m_context.value;}
	//TokenType tokenType() const {return m_context.tokenType;}

	virtual TokenType nextToken() = 0;
	void unget();
	//std::istream &in() {return m_in;}
protected:
	void pushContext();
	void popContext();

	bool isWithinMap() const {return m_context.containerType == ContainerType::Map
						   || m_context.containerType == ContainerType::IMap
						   || m_context.containerType == ContainerType::MetaData;}
protected:
	std::istream &m_in;
	Context m_context;
	std::vector<Context> m_contextStack;
};

} // namespace chainpack
} // namespace shv

