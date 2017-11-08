#include "rpcdriver.h"
#include "metatypes.h"
#include "chainpackprotocol.h"
#include "../core/shvexception.h"
#include "../core/log.h"

#include <sstream>
#include <iostream>

#define logRpc() shvCDebug("rpc")

namespace shv {
namespace core {
namespace chainpack {

int RpcDriver::s_defaultRpcTimeout = 5000;

RpcDriver::RpcDriver()
{
}

RpcDriver::~RpcDriver()
{
}

void RpcDriver::sendMessage(const shv::core::chainpack::RpcValue &msg)
{
	using namespace std;
	//shvLogFuncFrame() << msg.toStdString();
	std::ostringstream os_packed_data;
	shv::core::chainpack::ChainPackProtocol::write(os_packed_data, msg);
	std::string packed_data = os_packed_data.str();
	logRpc() << "send message: packed data: " << (packed_data.size() > 50? "<... long data ...>" : packed_data);

	m_highPriorityQueue.push_back(Chunk{std::move(packed_data)});

	writePendingData();
}

void RpcDriver::writePendingData()
{
	if(!isOpen()) {
		shvError() << "write data error, socket is not open!";
		return;
	}
	if(bytesToWrite() > 0) {
		logRpc() << "skipping write because of bytesToWrite:" << bytesToWrite() << "try to flush";
		bool ok = flushNoBlock();
		logRpc() << "any data flushed:" << ok << "rest bytesToWrite:" << bytesToWrite();
		return;
	}
	if(!m_highPriorityQueue.empty()) {
		logRpc() << "writePendingData(), HI prio queue len:" << m_highPriorityQueue.size();
		writeQueue(m_highPriorityQueue, m_headChunkBytesWrittenSoFar);
	}
}

namespace {
const unsigned PROTOCOL_VERSION = 1;
}

void RpcDriver::writeQueue(std::deque<Chunk> &queue, size_t &bytes_written_so_far)
{
	//static int hi_cnt = 0;
	const Chunk &chunk = queue[0];

	if(bytes_written_so_far == 0) {
		std::string protocol_version_data;
		{
			std::ostringstream os;
			shv::core::chainpack::ChainPackProtocol::writeUIntData(os, PROTOCOL_VERSION);
			protocol_version_data = os.str();
		}
		{
			std::ostringstream os;
			shv::core::chainpack::ChainPackProtocol::writeUIntData(os, chunk.data.length() + protocol_version_data.length());
			std::string packet_len_data = os.str();
			auto len = writeBytes(packet_len_data.data(), packet_len_data.length());
			if(len < 0)
				SHV_EXCEPTION("Write socket error!");
			if(len < (int)packet_len_data.length())
				SHV_EXCEPTION("Design error! Chunk length shall be always written at once to the socket");
		}
		{
			auto len = writeBytes(protocol_version_data.data(), protocol_version_data.length());
			if(len < 0)
				SHV_EXCEPTION("Write socket error!");
			if(len != 1)
				SHV_EXCEPTION("Design error! Protocol version shall be always written at once to the socket");
		}
	}

	{
		//int data_written_so_far;
		const std::string &data = chunk.data;
		auto len = writeBytes(data.data() + bytes_written_so_far, data.length() - bytes_written_so_far);
		if(len < 0)
			SHV_EXCEPTION("Write socket error!");
		if(len == 0)
			SHV_EXCEPTION("Design error! At least 1 byte of data shall be always written to the socket");

		logRpc() << "writeQueue - data len:" << data.length() << "start index:" << bytes_written_so_far << "bytes written:" << len << "remaining:" << (data.length() - bytes_written_so_far - len);
		bytes_written_so_far += len;
		if(bytes_written_so_far == chunk.data.length()) {
			bytes_written_so_far = 0;
			queue.pop_front();
		}
	}
}

void RpcDriver::bytesRead(std::string &&bytes)
{
	logRpc() << bytes.length() << "bytes of data read";
	m_readData += std::string(std::move(bytes));
	while(true) {
		int len = processReadData(m_readData);
		//shvInfo() << len << "bytes of" << m_readData.size() << "processed";
		if(len > 0)
			m_readData = m_readData.substr(len);
		else
			break;
	}
}

int RpcDriver::processReadData(const std::string &read_data)
{
	logRpc() << __FUNCTION__ << "data len:" << read_data.length();
	using namespace shv::core::chainpack;

	std::istringstream in(read_data);
	bool ok;

	uint64_t chunk_len = ChainPackProtocol::readUIntData(in, &ok);
	if(!ok)
		return 0;

	size_t read_len = (size_t)in.tellg() + chunk_len;

	uint64_t protocol_version = ChainPackProtocol::readUIntData(in, &ok);
	if(!ok)
		return 0;

	if(protocol_version != PROTOCOL_VERSION)
		SHV_EXCEPTION("Unsupported protocol version");

	logRpc() << "\t chunk len:" << chunk_len << "read_len:" << read_len << "stream pos:" << in.tellg();
	if(read_len > read_data.length())
		return 0;

	RpcValue msg = ChainPackProtocol::read(in);
	onMessageReceived(msg);
	return read_len;
}

void RpcDriver::onMessageReceived(const RpcValue &msg)
{
	logRpc() << "\t emitting message received:" << msg.toStdString();
	//logLongFiles() << "\t emitting message received:" << msg.dumpText();
	if(m_messageReceivedCallback)
		m_messageReceivedCallback(msg);
}

} // namespace chainpack
} // namespace core
} // namespace shv