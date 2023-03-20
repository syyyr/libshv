#pragma once

#include "../shvchainpackglobal.h"
#include "rpcmessage.h"
#include "rpc.h"
//#include "abstractstreamreader.h"

#include <functional>
#include <string>
#include <deque>
#include <map>

namespace shv {
namespace chainpack {

class ParseException;

class SHVCHAINPACK_DECL_EXPORT RpcDriver
{
public:
	static constexpr auto SND_LOG_ARROW = "<==S";
	static constexpr auto RCV_LOG_ARROW = "R==>";
public:
	explicit RpcDriver();
	virtual ~RpcDriver();

	Rpc::ProtocolType protocolType() const {return m_protocolType;}
	void setProtocolType(Rpc::ProtocolType v) {m_protocolType = v;}

	void sendRpcValue(const RpcValue &msg);
	void sendRawData(std::string &&data);
	virtual void sendRawData(const RpcValue::MetaData &meta_data, std::string &&data);

	static int defaultRpcTimeoutMsec() {return s_defaultRpcTimeoutMsec;}
	static void setDefaultRpcTimeoutMsec(int msec) {s_defaultRpcTimeoutMsec = msec;}

	static RpcMessage composeRpcMessage(RpcValue::MetaData &&meta_data, const std::string &data, std::string *errmsg = nullptr);

	static size_t decodeMetaData(RpcValue::MetaData &meta_data, Rpc::ProtocolType protocol_type, const std::string &data, size_t start_pos);
	static RpcValue decodeData(Rpc::ProtocolType protocol_type, const std::string &data, size_t start_pos);
	static std::string codeRpcValue(Rpc::ProtocolType protocol_type, const RpcValue &val);

	static std::string dataToPrettyCpon(shv::chainpack::Rpc::ProtocolType protocol_type, const shv::chainpack::RpcValue::MetaData &md, const std::string &data, size_t start_pos = 0, size_t data_len = 0);
protected:
	struct MessageData
	{
		std::string metaData;
		std::string data;

		MessageData() = default;
		MessageData(std::string &&meta_data_, std::string &&data_) : metaData(std::move(meta_data_)), data(std::move(data_)) {}
		MessageData(std::string &&data_) : data(std::move(data_)) {}
		MessageData(MessageData &&) = default;

		bool empty() const {return metaData.empty() && data.empty();}
		size_t size() const {return metaData.size() + data.size();}
	};
protected:
	virtual bool isOpen() = 0;

	/// called before RpcMessage data is about to written
	virtual void writeMessageBegin() = 0;
	/// called after RpcMessage data is written
	/// data should be flushed in derived class implementation
	virtual void writeMessageEnd() = 0;
	/// write bytes to write buffer (and possibly to socket)
	/// @return number of writen bytes
	virtual int64_t writeBytes(const char *bytes, size_t length) = 0;
	/// call it when new data arrived
	virtual void onBytesRead(std::string &&bytes);
	/// flush write buffer to socket
	/// @return true if write buffer length has changed (some data was written to the socket)
	//virtual bool flush() = 0;

	virtual void clearBuffers();

	/// add data to the output queue, send data from top of the queue
	virtual void enqueueDataToSend(MessageData &&chunk_to_enqueue);

	virtual void onRpcDataReceived(Rpc::ProtocolType protocol_type, RpcValue::MetaData &&md, std::string &&data);
	virtual void onParseDataException(const shv::chainpack::ParseException &e) = 0;
	virtual void onRpcValueReceived(const RpcValue &msg);
	virtual void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg) = 0;

	void lockSendQueueGuard();
	void unlockSendQueueGuard();
private:
	void processReadData();
	void writeQueue();
	int64_t writeBytes_helper(const std::string &str, size_t from, size_t length);
private:
	//MessageReceivedCallback m_messageReceivedCallback = nullptr;
	std::deque<MessageData> m_sendQueue;
	bool m_topMessageDataHeaderWritten = false;
	size_t m_topMessageDataBytesWrittenSoFar = 0;
	std::string m_readData;
	Rpc::ProtocolType m_protocolType = Rpc::ProtocolType::Invalid;
	static int s_defaultRpcTimeoutMsec;
	bool m_skipCorruptedHeaders = false;
};

} // namespace chainpack
} // namespace shv
