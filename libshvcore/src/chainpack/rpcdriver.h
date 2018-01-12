#pragma once

#include "../shvcoreglobal.h"
#include "rpcmessage.h"

#include <functional>
#include <string>
#include <deque>
#include <map>

namespace shv {
namespace core {
namespace chainpack {

class SHVCORE_DECL_EXPORT RpcDriver
{
public:
	explicit RpcDriver();
	virtual ~RpcDriver();

	void sendMessage(const RpcValue &msg);
	void sendRawData(std::string &&data);
	using MessageReceivedCallback = std::function< void (const shv::core::chainpack::RpcValue &msg)>;
	void setMessageReceivedCallback(const MessageReceivedCallback &callback) {m_messageReceivedCallback = callback;}

	static int defaultRpcTimeout() {return s_defaultRpcTimeout;}
	static void setDefaultRpcTimeout(int tm) {s_defaultRpcTimeout = tm;}
protected:
	using Chunk = std::string;
protected:
	virtual bool isOpen() = 0;
	/// write bytes to write buffer (and possibly to socket)
	/// @return number of writen bytes
	virtual int64_t writeBytes(const char *bytes, size_t length) = 0;
	/// call it when new data arrived
	void onBytesRead(std::string &&bytes);
	/// flush write buffer to socket
	/// @return true if write buffer length has changed (some data was written to the socket)
	virtual bool flush() = 0;

	/// add data to the output queue, send data from top of the queue
	virtual void enqueueDataToSend(Chunk &&chunk_to_enqueue);
	virtual void onMessageReceived(const shv::core::chainpack::RpcValue &msg);
private:
	int processReadData(const std::string &read_data);
	void writeQueue();
private:
	MessageReceivedCallback m_messageReceivedCallback = nullptr;

	std::deque<Chunk> m_chunkQueue;
	size_t m_headChunkBytesWrittenSoFar = 0;
	std::string m_readData;
	static int s_defaultRpcTimeout;
};

} // namespace chainpack
} // namespace core
} // namespace shv
