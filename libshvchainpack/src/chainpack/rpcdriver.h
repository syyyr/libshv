#pragma once

#include "../shvchainpackglobal.h"
#include "rpcmessage.h"
#include "rpc.h"

#include <functional>
#include <string>
#include <deque>
#include <map>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT RpcDriver
{
public:
	static const char * SND_LOG_ARROW;
	static const char * RCV_LOG_ARROW;
public:
	explicit RpcDriver();
	virtual ~RpcDriver();

	Rpc::ProtocolType protocolType() const {return m_protocolType;}
	void setProtocolType(Rpc::ProtocolType v) {m_protocolType = v;}

	void sendRpcValue(const RpcValue &msg);
	void sendRawData(std::string &&data);
	virtual void sendRawData(const RpcValue::MetaData &meta_data, std::string &&data);
	using MessageReceivedCallback = std::function< void (const RpcValue &msg)>;
	void setMessageReceivedCallback(const MessageReceivedCallback &callback) {m_messageReceivedCallback = callback;}

	static int defaultRpcTimeout() {return s_defaultRpcTimeout;}
	static void setDefaultRpcTimeout(int tm) {s_defaultRpcTimeout = tm;}

	static RpcMessage composeRpcMessage(RpcValue::MetaData &&meta_data, const std::string &data, std::string *errmsg = nullptr);

	static size_t decodeMetaData(RpcValue::MetaData &meta_data, Rpc::ProtocolType protocol_type, const std::string &data, size_t start_pos);
	static RpcValue decodeData(Rpc::ProtocolType protocol_type, const std::string &data, size_t start_pos);
	static std::string codeRpcValue(Rpc::ProtocolType protocol_type, const RpcValue &val);
protected:
	struct Chunk
	{
		std::string metaData;
		std::string data;

		Chunk() {}
		Chunk(std::string &&meta_data, std::string &&data) : metaData(std::move(meta_data)), data(std::move(data)) {}
		Chunk(std::string &&data) : data(std::move(data)) {}
		Chunk(Chunk &&) = default;

		bool empty() const {return metaData.empty() && data.empty();}
		size_t size() const {return metaData.size() + data.size();}
	};
protected:
	virtual bool isOpen() = 0;
	/// write bytes to write buffer (and possibly to socket)
	/// @return number of writen bytes
	virtual int64_t writeBytes(const char *bytes, size_t length) = 0;
	/// call it when new data arrived
	virtual void onBytesRead(std::string &&bytes);
	/// flush write buffer to socket
	/// @return true if write buffer length has changed (some data was written to the socket)
	virtual bool flush() = 0;

	/// add data to the output queue, send data from top of the queue
	virtual void enqueueDataToSend(Chunk &&chunk_to_enqueue);

	virtual void onRpcDataReceived(Rpc::ProtocolType protocol_type, RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len);
	virtual void onRpcValueReceived(const RpcValue &msg);
	virtual void onProcessReadDataException(std::exception &e) = 0;

	virtual void lockSendQueue() {}
	virtual void unlockSendQueue() {}
private:
	int processReadData(const std::string &read_data);
	void writeQueue();
	int64_t writeBytes_helper(const std::string &str, size_t from, size_t length);
private:
	MessageReceivedCallback m_messageReceivedCallback = nullptr;
	std::deque<Chunk> m_chunkQueue;
	bool m_topChunkHeaderWritten = false;
	size_t m_topChunkBytesWrittenSoFar = 0;
	std::string m_readData;
	Rpc::ProtocolType m_protocolType = Rpc::ProtocolType::Invalid;
	static int s_defaultRpcTimeout;
};

} // namespace chainpack
} // namespace shv
