#include "rpcmessage.h"
#include "chainpackprotocol.h"
#include "metatypes.h"

#include <cassert>

namespace shv {
namespace core {
namespace chainpack {

namespace meta {
RpcMessage::RpcMessage()
	: Super("RpcMessage")
{
	m_keys = {
		{(int)Key::Method, {(int)Key::Method, "Method"}},
		{(int)Key::Params, {(int)Key::Params, "Params"}},
		{(int)Key::Result, {(int)Key::Result, "Result"}},
		{(int)Key::Error, {(int)Key::Error, "Error"}},
		{(int)Key::ErrorCode, {(int)Key::ErrorCode, "ErrorCode"}},
		{(int)Key::ErrorMessage, {(int)Key::ErrorMessage, "ErrorMessage"}},
	};
	m_tags = {
		{(int)Tag::RequestId, {(int)Tag::RequestId, "RequestId"}},
		{(int)Tag::RpcCallType, {(int)Tag::RpcCallType, "RpcCallType"}},
		{(int)Tag::ShvPath, {(int)Tag::ShvPath, "ShvPath"}},
	};
}

void RpcMessage::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static RpcMessage s;
		meta::registerType(meta::ChainPackNS::ID, meta::RpcMessage::ID, &s);
	}
}

}

//==================================================================
// RpcMessage
//==================================================================
RpcMessage::RpcMessage()
{
	meta::RpcMessage::registerMetaType();
}

RpcMessage::RpcMessage(const RpcValue &val)
	: RpcMessage()
{
	assert(val.isIMap());
	m_value = val;
}

bool RpcMessage::hasKey(RpcValue::UInt key) const
{
	return m_value.toIMap().count(key);
}

RpcValue RpcMessage::value(RpcValue::UInt key) const
{
	return m_value.at(key);
}

void RpcMessage::setValue(RpcValue::UInt key, const RpcValue &val)
{
	assert(key >= meta::RpcMessage::Key::Method && key < meta::RpcMessage::Key::MAX);
	checkMetaValues();
	m_value.set(key, val);
}

RpcValue RpcMessage::metaValue(RpcValue::UInt key) const
{
	return m_value.metaValue(key);
}

void RpcMessage::setMetaValue(RpcValue::UInt key, const RpcValue &val)
{
	checkMetaValues();
	m_value.setMetaValue(key, val);
}

RpcValue::UInt RpcMessage::id() const
{
	if(isValid())
		return m_value.metaData().value(meta::RpcMessage::Tag::RequestId).toUInt();
	return 0;
}

void RpcMessage::setId(RpcValue::UInt id)
{
	checkMetaValues();
	checkRpcTypeMetaValue();
	setMetaValue(meta::RpcMessage::Tag::RequestId, id);
}

bool RpcMessage::isValid() const
{
	return m_value.isValid();
}

bool RpcMessage::isRequest() const
{
	return rpcType() == meta::RpcMessage::RpcCallType::Request;
}

bool RpcMessage::isNotify() const
{
	return rpcType() == meta::RpcMessage::RpcCallType::Notify;
}

RpcValue RpcMessage::shvPath() const
{
	return metaValue(meta::RpcMessage::Tag::ShvPath);
}

void RpcMessage::setShvPath(const RpcValue &path)
{
	setMetaValue(meta::RpcMessage::Tag::ShvPath, path);
}

bool RpcMessage::isResponse() const
{
	return rpcType() == meta::RpcMessage::RpcCallType::Response;
}

int RpcMessage::write(std::ostream &out) const
{
	assert(m_value.isValid());
	assert(rpcType() != meta::RpcMessage::RpcCallType::Undefined);
	return ChainPackProtocol::write(out, m_value);
}

meta::RpcMessage::RpcCallType::Enum RpcMessage::rpcType() const
{
	RpcValue::UInt rpc_id = id();
	bool has_method = hasKey(meta::RpcMessage::Key::Method);
	if(has_method)
		return (rpc_id > 0)? meta::RpcMessage::RpcCallType::Request: meta::RpcMessage::RpcCallType::Notify;
	if(hasKey(meta::RpcMessage::Key::Result) || hasKey(meta::RpcMessage::Key::Error))
		return meta::RpcMessage::RpcCallType::Response;
	return meta::RpcMessage::RpcCallType::Undefined;
}

void RpcMessage::checkMetaValues()
{
	if(!m_value.isValid()) {
		m_value = RpcValue::IMap();
		/// not needed, Global is default name space
		setMetaValue(meta::Tag::MetaTypeNameSpaceId, meta::ChainPackNS::ID);
		setMetaValue(meta::Tag::MetaTypeId, meta::RpcMessage::ID);
	}
}

void RpcMessage::checkRpcTypeMetaValue()
{
	meta::RpcMessage::RpcCallType::Enum rpc_type = isResponse()? meta::RpcMessage::RpcCallType::Response: isNotify()? meta::RpcMessage::RpcCallType::Notify: meta::RpcMessage::RpcCallType::Request;
	setMetaValue(meta::RpcMessage::Tag::RpcCallType, rpc_type);
}

std::string RpcMessage::toStdString() const
{
	return m_value.toStdString();
}

//==================================================================
// RpcRequest
//==================================================================
RpcValue::String RpcRequest::method() const
{
	return value(meta::RpcMessage::Key::Method).toString();
}

RpcRequest &RpcRequest::setMethod(RpcValue::String &&met)
{
	setValue(meta::RpcMessage::Key::Method, RpcValue{std::move(met)});
	checkRpcTypeMetaValue();
	return *this;
}

RpcValue RpcRequest::params() const
{
	return value(meta::RpcMessage::Key::Params);
}

RpcRequest& RpcRequest::setParams(const RpcValue& p)
{
	setValue(meta::RpcMessage::Key::Params, p);
	return *this;
}

//==================================================================
// RpcNotify
//==================================================================
void RpcNotify::write(std::ostream &out, const std::string &method, std::function<void (std::ostream &)> write_params_callback)
{
	RpcValue::MetaData md;
	md.setMetaTypeId(meta::RpcMessage::ID);
	md.setValue(meta::RpcMessage::Tag::RpcCallType, meta::RpcMessage::RpcCallType::Notify);
	ChainPackProtocol::writeMetaData(out, md);
	ChainPackProtocol::writeContainerBegin(out, RpcValue::Type::IMap);
	ChainPackProtocol::writeMapElement(out, meta::RpcMessage::Key::Method, method);
	ChainPackProtocol::writeUIntData(out, meta::RpcMessage::Key::Params);
	write_params_callback(out);
	ChainPackProtocol::writeContainerEnd(out);
}

//==================================================================
// RpcResponse
//==================================================================
RpcResponse::Error RpcResponse::error() const
{
	return Error{value(meta::RpcMessage::Key::Error).toIMap()};
}

RpcResponse &RpcResponse::setError(RpcResponse::Error err)
{
	setValue(meta::RpcMessage::Key::Error, std::move(err));
	checkRpcTypeMetaValue();
	return *this;
}

RpcValue RpcResponse::result() const
{
	return value(meta::RpcMessage::Key::Result);
}

RpcResponse& RpcResponse::setResult(const RpcValue& res)
{
	setValue(meta::RpcMessage::Key::Result, res);
	checkRpcTypeMetaValue();
	return *this;
}

RpcResponse::Error::ErrorType RpcResponse::Error::code() const
{
	auto iter = find(KeyCode);
	return (iter == end()) ? NoError : (ErrorType)(iter->second.toUInt());
}

RpcResponse::Error& RpcResponse::Error::setCode(ErrorType c)
{
	(*this)[KeyCode] = RpcValue{(RpcValue::UInt)c};
	return *this;
}

RpcResponse::Error& RpcResponse::Error::setMessage(RpcValue::String &&mess)
{
	(*this)[KeyMessage] = RpcValue{std::move(mess)};
	return *this;
}

RpcValue::String RpcResponse::Error::message() const
{
	auto iter = find(KeyMessage);
	return (iter == end()) ? RpcValue::String{} : iter->second.toString();
}

} // namespace chainpackrpc
}
} // namespace shv
