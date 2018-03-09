#pragma once

#include "rpcvalue.h"
#include "rpc.h"

#include "utils.h"
#include "../shvchainpackglobal.h"

#include <functional>

namespace shv {
namespace chainpack {

namespace meta {

class RpcMessage : public meta::MetaType
{
	using Super = meta::MetaType;
public:
	enum {ID = 1};
	struct Tag { enum Enum {RequestId = meta::Tag::USER,
							Destination,
							Method,
							CallerId,
							ProtocolType, //needed when dest client is using different version than source one to translate raw message data to correct format
							MAX};};
	struct Key { enum Enum {Params = 1, Result, Error, ErrorCode, ErrorMessage, MAX};};

	RpcMessage();

	static void registerMetaType();
};

}

class AbstractStreamWriter;

class SHVCHAINPACK_DECL_EXPORT RpcMessage
{
public:
	RpcMessage();
	RpcMessage(const RpcValue &val);
	const RpcValue& value() const {return m_value;}
protected:
	bool hasKey(RpcValue::UInt key) const;
	RpcValue value(RpcValue::UInt key) const;
	void setValue(RpcValue::UInt key, const RpcValue &val);
public:
	bool isValid() const;
	bool isRequest() const;
	bool isResponse() const;
	bool isNotify() const;

	static bool isRequest(const RpcValue::MetaData &meta);
	static bool isResponse(const RpcValue::MetaData &meta);
	static bool isNotify(const RpcValue::MetaData &meta);

	static RpcValue::UInt requestId(const RpcValue::MetaData &meta);
	static void setRequestId(RpcValue::MetaData &meta, RpcValue::UInt requestId);
	RpcValue::UInt requestId() const;
	void setRequestId(RpcValue::UInt requestId);

	static RpcValue::String method(const RpcValue::MetaData &meta);
	static void setMethod(RpcValue::MetaData &meta, const RpcValue::String &method);
	RpcValue::String method() const;
	void setMethod(const RpcValue::String &method);

	static RpcValue::String destination(const RpcValue::MetaData &meta);
	static void setDestination(RpcValue::MetaData &meta, const RpcValue::String &path);
	RpcValue::String destination() const;
	void setDestination(const RpcValue::String &path);

	static RpcValue callerId(const RpcValue::MetaData &meta);
	static void setCallerId(RpcValue::MetaData &meta, const RpcValue &caller_id);
	static void pushCallerId(RpcValue::MetaData &meta, RpcValue::UInt caller_id);
	static RpcValue::UInt popCallerId(RpcValue::MetaData &meta);
	RpcValue callerId() const;
	void setCallerId(const RpcValue &callerId);

	static Rpc::ProtocolType protocolType(const RpcValue::MetaData &meta);
	static void setProtocolType(RpcValue::MetaData &meta, shv::chainpack::Rpc::ProtocolType ver);
	Rpc::ProtocolType protocolType() const;
	void setProtocolType(shv::chainpack::Rpc::ProtocolType ver);

	std::string toPrettyString() const;
	std::string toCpon() const;

	RpcValue metaValue(RpcValue::UInt key) const;
	void setMetaValue(RpcValue::UInt key, const RpcValue &val);

	virtual size_t write(AbstractStreamWriter &wr) const;
protected:
	//enum class RpcCallType { Undefined = 0, Request, Response, Notify };
	//RpcCallType rpcType() const;
	void checkMetaValues();
	//void checkRpcTypeMetaValue();
protected:
	RpcValue m_value;
};

class SHVCHAINPACK_DECL_EXPORT RpcRequest : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	RpcRequest() : Super() {}
	//RpcRequest(const Value &id) : Super(Json()) {setId(id);}
	RpcRequest(const RpcMessage &msg) : Super(msg) {}
public:
	RpcRequest& setMethod(const RpcValue::String &met);
	RpcRequest& setMethod(RpcValue::String &&met);
	//RpcValue::String method() const;
	RpcRequest& setParams(const RpcValue &p);
	RpcValue params() const;
	RpcRequest& setRequestId(const RpcValue::UInt id) {Super::setRequestId(id); return *this;}

	//size_t write(AbstractStreamWriter &wr) const override;
};

class SHVCHAINPACK_DECL_EXPORT RpcNotify : public RpcRequest
{
private:
	using Super = RpcRequest;
public:
	RpcNotify() : Super() {}
	//RpcRequest(const Value &id) : Super(Json()) {setId(id);}
	RpcNotify(const RpcMessage &msg) : Super(msg) {}
public:
	RpcRequest& setRequestId(const RpcValue::UInt requestId) = delete;

	static void write(AbstractStreamWriter &wr, const std::string &method, std::function<void (AbstractStreamWriter &)> write_params_callback);
};

class SHVCHAINPACK_DECL_EXPORT RpcResponse : public RpcMessage
{
private:
	using Super = RpcMessage;
public:
	class SHVCHAINPACK_DECL_EXPORT Error : public RpcValue::IMap
	{
	private:
		using Super = RpcValue::IMap;
		enum {KeyCode = 1, KeyMessage};
	public:
		enum ErrorType {
			NoError = 0,
			InvalidRequest,	// The JSON sent is not a valid Request object.
			MethodNotFound,	// The method does not exist / is not available.
			InvalidParams,		// Invalid method parameter(s).
			InternalError,		// Internal JSON-RPC error.
			ParseError,		// Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text.
			SyncMethodCallTimeout,
			SyncMethodCallCancelled,
			MethodInvocationException,
			Unknown
		};
	public:
		Error(const Super &m = Super()) : Super(m) {}
		Error& setCode(ErrorType c);
		ErrorType code() const;
		Error& setMessage(RpcValue::String &&m);
		RpcValue::String message() const;
		//Error& setData(const Value &data);
		//Value data() const;
		RpcValue::String toString() const {return "RPC ERROR " + Utils::toString(code()) + ": " + message();}
	public:
		static Error create(ErrorType c, RpcValue::String msg) {
			Error ret;
			ret.setCode(c).setMessage(std::move(msg));
			return ret;
		}
		/*
		static Error createParseError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(ParseError,
							   (msg.isEmpty())? "Parse error": msg,
							   (data.isValid())? data: "Invalid JSON was received by the server. An error occurred on the server while parsing the JSON text.");
		}
		static Error createInvalidRequestError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(InvalidRequest,
							   (msg.isEmpty())? "Invalid Request": msg,
							   (data.isValid())? data: "The JSON sent is not a valid Request object.");
		}
		static Error createMethodNotFoundError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(MethodNotFound,
							   (msg.isEmpty())? "Method not found": msg,
							   (data.isValid())? data: "The method does not exist / is not available.");
		}
		static Error createInvalidParamsError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(InvalidParams,
							   (msg.isEmpty())? "Invalid params": msg,
							   (data.isValid())? data: "Invalid method parameter(s).");
		}
		static Error createMethodInvocationExceptionError(const Value::String &msg = Value::String(), const Value &data = Value()) {
			return createError(MethodInvocationException, msg, data);
		}
		*/
		static Error createInternalError(const RpcValue::String &msg = RpcValue::String()) {
			return create(InternalError, (msg.empty())? "Internal error": msg);
		}
		static Error createSyncMethodCallTimeout(const RpcValue::String &msg = RpcValue::String()) {
			return create(SyncMethodCallTimeout, (msg.empty())? "Sync method call timeout": msg);
		}
	};
public:
	//RpcResponse(const Json &json = Json()) : Super(json) {}
	//RpcResponse(const Value &request_id) : Super(Json()) { setId(request_id); }
	RpcResponse() : Super() {}
	RpcResponse(const RpcMessage &msg) : Super(msg) {}
	static RpcResponse forRequest(const RpcRequest &rq);
public:
	bool isError() const {return !error().empty();}
	RpcResponse& setError(Error err);
	Error error() const;
	RpcResponse& setResult(const RpcValue &res);
	RpcValue result() const;
	RpcResponse& setRequestId(const RpcValue::UInt id) {Super::setRequestId(id); return *this;}
};

} // namespace chainpackrpc
} // namespace shv
