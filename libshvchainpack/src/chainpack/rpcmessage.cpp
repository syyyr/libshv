#include "rpcmessage.h"
#include "chainpack.h"
#include "metatypes.h"
#include "abstractstreamwriter.h"

#include <cassert>

namespace shv {
namespace chainpack {

RpcMessage::MetaType::MetaType()
	: Super("RpcMessage")
{
	m_keys = {
		{(int)Key::Params, {(int)Key::Params, "params"}},
		{(int)Key::Result, {(int)Key::Result, "result"}},
		{(int)Key::Error, {(int)Key::Error, "error"}},
		{(int)Key::ErrorCode, {(int)Key::ErrorCode, "errorCode"}},
		{(int)Key::ErrorMessage, {(int)Key::ErrorMessage, "errorMessage"}},
	};
	m_tags = {
		{(int)Tag::RequestId, {(int)Tag::RequestId, "id"}},
		{(int)Tag::ShvPath, {(int)Tag::ShvPath, "shvPath"}},
		{(int)Tag::Method, {(int)Tag::Method, "method"}},
		{(int)Tag::CallerIds, {(int)Tag::CallerIds, "cid"}},
		{(int)Tag::ProtocolType, {(int)Tag::ProtocolType, "protocol"}},
		{(int)Tag::RevCallerIds, {(int)Tag::RevCallerIds, "rcid"}},
	};
}

void RpcMessage::MetaType::registerMetaType()
{
	static bool is_init = false;
	if(!is_init) {
		is_init = true;
		static MetaType s;
		meta::registerType(meta::GlobalNS::ID, MetaType::ID, &s);
	}
}

//==================================================================
// RpcMessage
//==================================================================
bool RpcMessage::m_isMetaTypeExplicit = false;

RpcMessage::RpcMessage()
{
	MetaType::registerMetaType();
}

RpcMessage::RpcMessage(const RpcValue &val)
	: RpcMessage()
{
	if(!val.isIMap())
		SHVCHP_EXCEPTION("Value is not IMap");
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
	assert(key >= RpcMessage::MetaType::Key::Params && key < RpcMessage::MetaType::Key::MAX);
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

RpcValue RpcMessage::requestId() const
{
	if(isValid())
		return requestId(m_value.metaData());
	return RpcValue();
}

void RpcMessage::setRequestId(const RpcValue &id)
{
	checkMetaValues();
	//checkRpcTypeMetaValue();
	setMetaValue(RpcMessage::MetaType::Tag::RequestId, id);
}

RpcValue RpcMessage::method(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::Method);
}

void RpcMessage::setMethod(RpcValue::MetaData &meta, const RpcValue::String &method)
{
	meta.setValue(RpcMessage::MetaType::Tag::Method, method);
}

RpcValue RpcMessage::method() const
{
	return metaValue(RpcMessage::MetaType::Tag::Method);
}

void RpcMessage::setMethod(const RpcValue::String &method)
{
	checkMetaValues();
	setMetaValue(RpcMessage::MetaType::Tag::Method, method);
}

bool RpcMessage::isValid() const
{
	return m_value.isValid();
}

bool RpcMessage::isRequest() const
{
	return requestId().isValid() && !method().toString().empty();
}

bool RpcMessage::isNotify() const
{
	return !requestId().isValid() && !method().toString().empty();
}

bool RpcMessage::isResponse() const
{
	return requestId().isValid() && method().toString().empty();
}

bool RpcMessage::isRequest(const RpcValue::MetaData &meta)
{
	return requestId(meta).isValid() && !method(meta).toString().empty();
}

bool RpcMessage::isResponse(const RpcValue::MetaData &meta)
{
	return requestId(meta).isValid() && method(meta).toString().empty();
}

bool RpcMessage::isNotify(const RpcValue::MetaData &meta)
{
	return !requestId(meta).isValid() && !method(meta).toString().empty();
}

RpcValue RpcMessage::requestId(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::RequestId);
}

void RpcMessage::setRequestId(RpcValue::MetaData &meta, const RpcValue &id)
{
	meta.setValue(RpcMessage::MetaType::Tag::RequestId, id);
}

RpcValue RpcMessage::shvPath(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::ShvPath);
}

void RpcMessage::setShvPath(RpcValue::MetaData &meta, const RpcValue::String &path)
{
	meta.setValue(RpcMessage::MetaType::Tag::ShvPath, path);
}

RpcValue RpcMessage::shvPath() const
{
	return metaValue(RpcMessage::MetaType::Tag::ShvPath);
}

void RpcMessage::setShvPath(const RpcValue::String &path)
{
	setMetaValue(RpcMessage::MetaType::Tag::ShvPath, path);
}

RpcValue RpcMessage::callerIds(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::CallerIds);
}

void RpcMessage::setCallerIds(RpcValue::MetaData &meta, const RpcValue &caller_id)
{
	meta.setValue(RpcMessage::MetaType::Tag::CallerIds, caller_id);
}

void RpcMessage::pushCallerId(RpcValue::MetaData &meta, RpcValue::Int caller_id)
{
	RpcValue curr_caller_id = RpcMessage::callerIds(meta);
	if(curr_caller_id.isList()) {
		RpcValue::List array = curr_caller_id.toList();
		array.push_back(RpcValue(caller_id));
		setCallerIds(meta, array);
	}
	else if(curr_caller_id.isUInt()) {
		RpcValue::List array;
		array.push_back(curr_caller_id.toInt());
		array.push_back(RpcValue(caller_id));
		setCallerIds(meta, array);
	}
	else {
		setCallerIds(meta, caller_id);
	}
}

RpcValue RpcMessage::popCallerId(const RpcValue &caller_ids, RpcValue::Int &id)
{
	if(caller_ids.isList()) {
		shv::chainpack::RpcValue::List array = caller_ids.toList();
		if(array.empty()) {
			id = 0;
		}
		else {
			id = array.back().toInt();
			array.pop_back();
		}
		return array;
	}
	else {
		id = caller_ids.toInt();
	}
	return RpcValue();
}

RpcValue::Int RpcMessage::popCallerId(RpcValue::MetaData &meta)
{
	RpcValue::Int ret = 0;
	setCallerIds(meta, popCallerId(callerIds(meta), ret));
	return ret;
}

RpcValue::Int RpcMessage::popCallerId()
{
	RpcValue::Int ret = 0;
	setCallerIds(popCallerId(callerIds(), ret));
	return ret;
}

RpcValue RpcMessage::callerIds() const
{
	return metaValue(RpcMessage::MetaType::Tag::CallerIds);
}

void RpcMessage::setCallerIds(const RpcValue &callerId)
{
	setMetaValue(RpcMessage::MetaType::Tag::CallerIds, callerId);
}

RpcValue RpcMessage::revCallerIds(const RpcValue::MetaData &meta)
{
	return meta.value(RpcMessage::MetaType::Tag::RevCallerIds);
}

void RpcMessage::setRevCallerIds(RpcValue::MetaData &meta, const RpcValue &caller_ids)
{
	meta.setValue(RpcMessage::MetaType::Tag::RevCallerIds, caller_ids);
}

void RpcMessage::pushRevCallerId(RpcValue::MetaData &meta, RpcValue::Int caller_id)
{
	RpcValue curr_caller_id = RpcMessage::revCallerIds(meta);
	if(curr_caller_id.isList()) {
		RpcValue::List array = curr_caller_id.toList();
		array.push_back(RpcValue(caller_id));
		setRevCallerIds(meta, array);
	}
	else if(curr_caller_id.isUInt()) {
		RpcValue::List array;
		array.push_back(curr_caller_id.toInt());
		array.push_back(RpcValue(caller_id));
		setRevCallerIds(meta, array);
	}
	else {
		setRevCallerIds(meta, caller_id);
	}
}

RpcValue RpcMessage::revCallerIds() const
{
	return metaValue(RpcMessage::MetaType::Tag::RevCallerIds);
}

void RpcMessage::setRegisterRevCallerIds()
{
	setMetaValue(RpcMessage::MetaType::Tag::RevCallerIds, nullptr);
}

bool RpcMessage::isRegisterRevCallerIds(const RpcValue::MetaData &meta)
{
	return revCallerIds(meta).isValid();
}

Rpc::ProtocolType RpcMessage::protocolType(const RpcValue::MetaData &meta)
{
	return (Rpc::ProtocolType)meta.value(RpcMessage::MetaType::Tag::ProtocolType).toUInt();
}

void RpcMessage::setProtocolType(RpcValue::MetaData &meta, Rpc::ProtocolType ver)
{
	meta.setValue(RpcMessage::MetaType::Tag::ProtocolType, ver == Rpc::ProtocolType::Invalid? RpcValue(): RpcValue((unsigned)ver));
}

Rpc::ProtocolType RpcMessage::protocolType() const
{
	return (Rpc::ProtocolType)metaValue(RpcMessage::MetaType::Tag::ProtocolType).toUInt();
}

void RpcMessage::setProtocolType(Rpc::ProtocolType ver)
{
	setMetaValue(RpcMessage::MetaType::Tag::ProtocolType, ver == Rpc::ProtocolType::Invalid? RpcValue(): RpcValue((unsigned)ver));
}

void RpcMessage::write(AbstractStreamWriter &wr) const
{
	assert(m_value.isValid());
	wr.write(m_value);
}

void RpcMessage::checkMetaValues()
{
	if(!m_value.isValid()) {
		m_value = RpcValue::IMap();
		/// not needed, Global is default name space
		//setMetaValue(meta::Tag::MetaTypeNameSpaceId, meta::GlobalNS::ID);
		/// not needed, RpcMessage is only type used so far
		if(m_isMetaTypeExplicit)
			setMetaValue(meta::Tag::MetaTypeId, RpcMessage::MetaType::ID);
	}
}

std::string RpcMessage::toPrettyString() const
{
	return m_value.toPrettyString();
}

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
	return value(RpcMessage::MetaType::Key::Method).toString();
}
*/
RpcRequest &RpcRequest::setMethod(const RpcValue::String &met)
{
	return setMethod(RpcValue::String(met));
}

RpcRequest &RpcRequest::setMethod(RpcValue::String &&met)
{
	setMetaValue(RpcMessage::MetaType::Tag::Method, RpcValue{std::move(met)});
	//checkRpcTypeMetaValue();
	return *this;
}

RpcValue RpcRequest::params() const
{
	return value(RpcMessage::MetaType::Key::Params);
}

RpcRequest& RpcRequest::setParams(const RpcValue& p)
{
	setValue(RpcMessage::MetaType::Key::Params, p);
	return *this;
}

//==================================================================
// RpcNotify
//==================================================================
void RpcNotify::write(AbstractStreamWriter &wr, const std::string &method, std::function<void (AbstractStreamWriter &)> write_params_callback)
{
	RpcValue::MetaData md;
	md.setMetaTypeId(RpcMessage::MetaType::ID);
	md.setValue(RpcMessage::MetaType::Tag::Method, method);
	wr.write(md);
	wr.writeContainerBegin(RpcValue::Type::IMap);
	wr.writeIMapKey(RpcMessage::MetaType::Key::Params);
	write_params_callback(wr);
	wr.writeContainerEnd(RpcValue::Type::IMap);
}

//==================================================================
// RpcResponse
//==================================================================
RpcResponse RpcResponse::forRequest(const RpcValue::MetaData &meta)
{
	RpcResponse ret;
	RpcValue id = requestId(meta);
	if(id.isValid())
		ret.setRequestId(id);
	auto caller_id = callerIds(meta);
	if(caller_id.isValid())
		ret.setCallerIds(caller_id);
	return ret;
}

RpcResponse::Error RpcResponse::error() const
{
	return Error{value(RpcMessage::MetaType::Key::Error).toIMap()};
}

RpcResponse &RpcResponse::setError(RpcResponse::Error err)
{
	setValue(RpcMessage::MetaType::Key::Error, std::move(err));
	//checkRpcTypeMetaValue();
	return *this;
}

RpcValue RpcResponse::result() const
{
	return value(RpcMessage::MetaType::Key::Result);
}

RpcResponse& RpcResponse::setResult(const RpcValue& res)
{
	setValue(RpcMessage::MetaType::Key::Result, res);
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
