#include "rpcmessage.h"
#include "chainpackprotocol.h"
#include "metatypes.h"

#include <cassert>

namespace shv {
namespace core {
namespace chainpack {

namespace tid {
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
		{(int)Tag::DeviceId, {(int)Tag::DeviceId, "DeviceId"}},
	};
}

void RpcMessage::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static RpcMessage s;
		MetaTypes::registerTypeId(ns::Default::ID, tid::RpcMessage::ID, &s);
	}
}

}

RpcMessage::RpcMessage()
{
	tid::RpcMessage::registerMetaType();
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
	assert(key >= tid::RpcMessage::Key::Method && key < tid::RpcMessage::Key::MAX);
	checkMetaValues();
	m_value.set(key, val);
}

void RpcMessage::setMetaValue(RpcValue::UInt key, const RpcValue &val)
{
	checkMetaValues();
	m_value.setMetaValue(key, val);
}

RpcValue::UInt RpcMessage::id() const
{
	if(isValid())
		return m_value.metaData().value(tid::RpcMessage::Tag::RequestId).toUInt();
	return 0;
}

void RpcMessage::setId(RpcValue::UInt id)
{
	checkMetaValues();
	checkRpcTypeMetaValue();
	setMetaValue(tid::RpcMessage::Tag::RequestId, id);
}

bool RpcMessage::isValid() const
{
	return m_value.isValid();
}

bool RpcMessage::isRequest() const
{
	return rpcType() == tid::RpcMessage::RpcCallType::Request;
}

bool RpcMessage::isNotify() const
{
	return rpcType() == tid::RpcMessage::RpcCallType::Notify;
}

bool RpcMessage::isResponse() const
{
	return rpcType() == tid::RpcMessage::RpcCallType::Response;
}

int RpcMessage::write(std::ostream &out) const
{
	assert(m_value.isValid());
	assert(rpcType() != tid::RpcMessage::RpcCallType::Undefined);
	return ChainPackProtocol::write(out, m_value);
}

tid::RpcMessage::RpcCallType::Enum RpcMessage::rpcType() const
{
	RpcValue::UInt rpc_id = id();
	bool has_method = hasKey(tid::RpcMessage::Key::Method);
	if(has_method)
		return (rpc_id > 0)? tid::RpcMessage::RpcCallType::Request: tid::RpcMessage::RpcCallType::Notify;
	if(hasKey(tid::RpcMessage::Key::Result) || hasKey(tid::RpcMessage::Key::Error))
		return tid::RpcMessage::RpcCallType::Response;
	return tid::RpcMessage::RpcCallType::Undefined;
}

void RpcMessage::checkMetaValues()
{
	if(!m_value.isValid()) {
		m_value = RpcValue::IMap();
		/// not needed, Global is default name space
		//setMetaValue(MetaTypes::Tag::MetaTypeNameSpaceId, MetaTypes::Default::Value);
		setMetaValue(MetaTypes::Tag::MetaTypeId, tid::RpcMessage::ID);
	}
}

void RpcMessage::checkRpcTypeMetaValue()
{
	tid::RpcMessage::RpcCallType::Enum rpc_type = isResponse()? tid::RpcMessage::RpcCallType::Response: isNotify()? tid::RpcMessage::RpcCallType::Notify: tid::RpcMessage::RpcCallType::Request;
	setMetaValue(tid::RpcMessage::Tag::RpcCallType, rpc_type);
}

RpcValue::String RpcRequest::method() const
{
	return value(tid::RpcMessage::Key::Method).toString();
}

RpcRequest &RpcRequest::setMethod(RpcValue::String &&met)
{
	setValue(tid::RpcMessage::Key::Method, RpcValue{std::move(met)});
	checkRpcTypeMetaValue();
	return *this;
}

RpcValue RpcRequest::params() const
{
	return value(tid::RpcMessage::Key::Params);
}

RpcRequest& RpcRequest::setParams(const RpcValue& p)
{
	setValue(tid::RpcMessage::Key::Params, p);
	return *this;
}

RpcResponse::Error RpcResponse::error() const
{
	return Error{value(tid::RpcMessage::Key::Error).toIMap()};
}

RpcResponse &RpcResponse::setError(RpcResponse::Error err)
{
	setValue(tid::RpcMessage::Key::Error, std::move(err));
	checkRpcTypeMetaValue();
	return *this;
}

RpcValue RpcResponse::result() const
{
	return value(tid::RpcMessage::Key::Result);
}

RpcResponse& RpcResponse::setResult(const RpcValue& res)
{
	setValue(tid::RpcMessage::Key::Result, res);
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

std::string RpcMessage::toStdString() const
{
	return m_value.toStdString();
}

} // namespace chainpackrpc
}
} // namespace shv
