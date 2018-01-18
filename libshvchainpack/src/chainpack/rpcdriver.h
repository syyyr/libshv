#pragma once

#include "../shvchainpackglobal.h"
#include "rpcmessage.h"

#include <functional>
#include <string>
#include <deque>
#include <map>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT RpcDriver
{
public:
	enum ProtocolVersion {ChainPack = 1, Cpon, Json};

	explicit RpcDriver();
	virtual ~RpcDriver();

	ProtocolVersion protocolVersion() const {return m_protocolVersion;}
	void setProtocolVersion(ProtocolVersion v) {m_protocolVersion = v;}

	void sendMessage(const RpcValue &msg);
	void sendRawData(std::string &&data);
	using MessageReceivedCallback = std::function< void (const RpcValue &msg)>;
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
	virtual void onMessageReceived(const RpcValue &msg);

	virtual void lockSendQueue() {}
	virtual void unlockSendQueue() {}
private:
	int processReadData(const std::string &read_data);
	void writeQueue();
private:
	MessageReceivedCallback m_messageReceivedCallback = nullptr;

	std::deque<Chunk> m_chunkQueue;
	size_t m_topChunkBytesWrittenSoFar = 0;
	std::string m_readData;
	ProtocolVersion m_protocolVersion = ChainPack;
	static int s_defaultRpcTimeout;
};

} // namespace chainpack
} // namespace shv
