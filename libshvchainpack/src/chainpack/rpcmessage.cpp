#include "rpcmessage.h"
#include "chainpack.h"
#include "metatypes.h"
#include "abstractstreamwriter.h"

#include <cassert>

namespace shv {
namespace chainpack {

namespace meta {
RpcMessage::RpcMessage()
	: Super("RpcMessage")
{
	m_keys = {
		{(int)Key::Params, {(int)Key::Params, "Params"}},
		{(int)Key::Result, {(int)Key::Result, "Result"}},
		{(int)Key::Error, {(int)Key::Error, "Error"}},
		{(int)Key::ErrorCode, {(int)Key::ErrorCode, "ErrorCode"}},
		{(int)Key::ErrorMessage, {(int)Key::ErrorMessage, "ErrorMessage"}},
	};
	m_tags = {
		{(int)Tag::RequestId, {(int)Tag::RequestId, "RequestId"}},
		{(int)Tag::CallerId, {(int)Tag::CallerId, "CallerId"}},
		{(int)Tag::Method, {(int)Tag::Method, "Method"}},
		{(int)Tag::ProtocolVersion, {(int)Tag::ProtocolVersion, "ProtocolVersion"}},
		{(int)Tag::ShvPath, {(int)Tag::ShvPath, "ShvPath"}},
	};
}

void RpcMessage::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static RpcMessage s;
		meta::registerType(meta::GlobalNS::ID, meta::RpcMessage::ID, &s);
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
	assert(key >= meta::RpcMessage::Key::Params && key < meta::RpcMessage::Key::MAX);
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

RpcValue::UInt RpcMessage::requestId() const
{
	if(isValid())
		return requestId(m_value.metaData());
	return 0;
}

void RpcMessage::setRequestId(RpcValue::UInt id)
{
	checkMetaValues();
	//checkRpcTypeMetaValue();
	setMetaValue(meta::RpcMessage::Tag::RequestId, id);
}

RpcValue::String RpcMessage::method(const RpcValue::MetaData &meta)
{
	return meta.value(meta::RpcMessage::Tag::Method).toString();
}

RpcValue::String RpcMessage::method() const
{
	return metaValue(meta::RpcMessage::Tag::Method).toString();
}

bool RpcMessage::isValid() const
{
	return m_value.isValid();
}

bool RpcMessage::isRequest() const
{
	return requestId() > 0 && !method().empty();
}

bool RpcMessage::isNotify() const
{
	return requestId() == 0 && !method().empty();
}

bool RpcMessage::isResponse() const
{
	return requestId() > 0 && method().empty();
}

bool RpcMessage::isRequest(const RpcValue::MetaData &meta)
{
	return requestId(meta) > 0 && !method(meta).empty();
}

bool RpcMessage::isResponse(const RpcValue::MetaData &meta)
{
	return requestId(meta) > 0 && method(meta).empty();
}

bool RpcMessage::isNotify(const RpcValue::MetaData &meta)
{
	return requestId(meta) == 0 && !method(meta).empty();
}

RpcValue::UInt RpcMessage::requestId(const RpcValue::MetaData &meta)
{
	return meta.value(meta::RpcMessage::Tag::RequestId).toUInt();
}

void RpcMessage::setRequestId(RpcValue::MetaData &meta, RpcValue::UInt id)
{
	meta.setValue(meta::RpcMessage::Tag::RequestId, id);
}

RpcValue::String RpcMessage::shvPath(const RpcValue::MetaData &meta)
{
	return meta.value(meta::RpcMessage::Tag::ShvPath).toString();
}

void RpcMessage::setShvPath(RpcValue::MetaData &meta, const RpcValue::String &path)
{
	meta.setValue(meta::RpcMessage::Tag::ShvPath, path);
}

RpcValue::String RpcMessage::shvPath() const
{
	return metaValue(meta::RpcMessage::Tag::ShvPath).toString();
}

void RpcMessage::setShvPath(const RpcValue::String &path)
{
	setMetaValue(meta::RpcMessage::Tag::ShvPath, path);
}

RpcValue::UInt RpcMessage::callerId(const RpcValue::MetaData &meta)
{
	return meta.value(meta::RpcMessage::Tag::CallerId).toUInt();
}

void RpcMessage::setCallerId(RpcValue::MetaData &meta, RpcValue::UInt id)
{
	meta.setValue(meta::RpcMessage::Tag::CallerId, id);
}

RpcValue::UInt RpcMessage::callerId() const
{
	return metaValue(meta::RpcMessage::Tag::CallerId).toUInt();
}

void RpcMessage::setCallerId(RpcValue::UInt id)
{
	setMetaValue(meta::RpcMessage::Tag::CallerId, id);
}

Rpc::ProtocolVersion RpcMessage::protocolVersion(const RpcValue::MetaData &meta)
{
	return (Rpc::ProtocolVersion)meta.value(meta::RpcMessage::Tag::ProtocolVersion).toUInt();
}

void RpcMessage::setProtocolVersion(RpcValue::MetaData &meta, Rpc::ProtocolVersion ver)
{
	meta.setValue(meta::RpcMessage::Tag::ProtocolVersion, ver == Rpc::ProtocolVersion::Invalid? RpcValue(): RpcValue((unsigned)ver));
}

Rpc::ProtocolVersion RpcMessage::protocolVersion() const
{
	return (Rpc::ProtocolVersion)metaValue(meta::RpcMessage::Tag::ProtocolVersion).toUInt();
}

void RpcMessage::setProtocolVersion(Rpc::ProtocolVersion ver)
{
	setMetaValue(meta::RpcMessage::Tag::ProtocolVersion, ver == Rpc::ProtocolVersion::Invalid? RpcValue(): RpcValue((unsigned)ver));
}

size_t RpcMessage::write(AbstractStreamWriter &wr) const
{
	assert(m_value.isValid());
	return wr.write(m_value);
}
/*
RpcMessage::RpcCallType RpcMessage::rpcType() const
{
	RpcValue::UInt rpc_id = requestId();
	bool has_method = !method().empty();
	if(has_method)
		return (rpc_id > 0)? RpcCallType::Request: RpcCallType::Notify;
	if(hasKey(meta::RpcMessage::Key::Result) || hasKey(meta::RpcMessage::Key::Error))
		return RpcCallType::Response;
	return RpcCallType::Undefined;
}
*/
void RpcMessage::checkMetaValues()
{
	if(!m_value.isValid()) {
		m_value = RpcValue::IMap();
		/// not needed, Global is default name space
		//setMetaValue(meta::Tag::MetaTypeNameSpaceId, meta::GlobalNS::ID);
		setMetaValue(meta::Tag::MetaTypeId, meta::RpcMessage::ID);
	}
}
/*
void RpcMessage::checkRpcTypeMetaValue()
{
	meta::RpcCallType::Enum rpc_type = isResponse()? meta::RpcCallType::Response: isNotify()? meta::RpcCallType::Notify: meta::RpcCallType::Request;
	setMetaValue(meta::RpcMessage::Tag::RpcCallType, rpc_type);
}
*/
std::string RpcMessage::toCpon() const
{
	return m_value.toCpon();
}

//==================================================================
// RpcRequest
//==================================================================
/*
RpcValue::String RpcRequest::method() const
{
	return value(meta::RpcMessage::Key::Method).toString();
}
*/
RpcRequest &RpcRequest::setMethod(const RpcValue::String &met)
{
	return setMethod(RpcValue::String(met));
}

RpcRequest &RpcRequest::setMethod(RpcValue::String &&met)
{
	setMetaValue(meta::RpcMessage::Tag::Method, RpcValue{std::move(met)});
	//checkRpcTypeMetaValue();
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
void RpcNotify::write(AbstractStreamWriter &wr, const std::string &method, std::function<void (AbstractStreamWriter &)> write_params_callback)
{
	RpcValue::MetaData md;
	md.setMetaTypeId(meta::RpcMessage::ID);
	md.setValue(meta::RpcMessage::Tag::Method, method);
	wr.write(md);
	wr.writeContainerBegin(RpcValue::Type::IMap);
	wr.writeIMapKey(meta::RpcMessage::Key::Params);
	write_params_callback(wr);
	wr.writeContainerEnd(RpcValue::Type::IMap);
}

//==================================================================
// RpcResponse
//==================================================================
RpcResponse RpcResponse::forRequest(const RpcRequest &rq)
{
	RpcResponse ret;
	RpcValue::UInt id = rq.requestId();
	if(id > 0)
		ret.setRequestId(id);
	id = rq.callerId();
	if(id > 0)
		ret.setCallerId(id);
	return ret;
}

RpcResponse::Error RpcResponse::error() const
{
	return Error{value(meta::RpcMessage::Key::Error).toIMap()};
}

RpcResponse &RpcResponse::setError(RpcResponse::Error err)
{
	setValue(meta::RpcMessage::Key::Error, std::move(err));
	//checkRpcTypeMetaValue();
	return *this;
}

RpcValue RpcResponse::result() const
{
	return value(meta::RpcMessage::Key::Result);
}

RpcResponse& RpcResponse::setResult(const RpcValue& res)
{
	setValue(meta::RpcMessage::Key::Result, res);
	//checkRpcTypeMetaValue();
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
} // namespace shv
