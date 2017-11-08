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
	using MessageReceivedCallback = std::function< void (const shv::core::chainpack::RpcValue &msg)>;
	void setMessageReceivedCallback(const MessageReceivedCallback &callback) {m_messageReceivedCallback = callback;}

	static int defaultRpcTimeout() {return s_defaultRpcTimeout;}
	static void setDefaultRpcTimeout(int tm) {s_defaultRpcTimeout = tm;}
protected:
	using Chunk = std::string;
protected:
	virtual bool isOpen() = 0;
	virtual size_t bytesToWrite() = 0;
	virtual int64_t writeBytes(const char *bytes, size_t length) = 0;
	virtual int64_t flushNoBlock() = 0;

	// call it when new data arrived
	void bytesRead(std::string &&bytes);
	// call it, when data are sent, when bytesToWrite() == 0
	virtual void writePendingData(Chunk &&chunk_to_enqueue);
	virtual void onMessageReceived(const shv::core::chainpack::RpcValue &msg);
private:
	/*
	struct Chunk
	{
		//std::string packedLength;
		//std::string packedHeader;
		std::string data;

		//Chunk(const ChunkHeader &h, std::string &&d) : packedHeader(h.toStringData()), data(std::move(d)) {fillPackedLength();}
		Chunk(std::string &&d) : data(std::move(d)) {}
		//Chunk(std::string &&h, std::string &&d) : packedHeader(std::move(h)), data(std::move(d)) {fillPackedLength();}

		//std::string packedLength() const;
		size_t length() const {return data.length();}
	};
	*/
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
