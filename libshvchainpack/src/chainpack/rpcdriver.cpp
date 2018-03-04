#include "rpcdriver.h"
#include "metatypes.h"
#include "chainpack.h"
#include "exception.h"
#include "cponwriter.h"
#include "cponreader.h"
#include "chainpackwriter.h"
#include "chainpackreader.h"

#include <necrolog.h>

#include <sstream>
#include <iostream>

#define logRpcMsg() nCDebug("RpcMsg")
#define logRpcData() nCDebug("RpcData")
//#define logRpcSyncCalls() nCDebug("RpcSyncCalls")

namespace shv {
namespace chainpack {

const char * RpcDriver::SND_LOG_ARROW = "<==";
const char * RpcDriver::RCV_LOG_ARROW = "==>";

int RpcDriver::s_defaultRpcTimeout = 5000;

RpcDriver::RpcDriver()
{
}

RpcDriver::~RpcDriver()
{
}

void RpcDriver::sendRpcValue(const RpcValue &msg)
{
	using namespace std;
	//shvLogFuncFrame() << msg.toStdString();
	logRpcMsg() << SND_LOG_ARROW << msg.toPrettyString();
	std::string packed_data = codeRpcValue(protocolVersion(), msg);
	logRpcData() << "send message:"
				 << ((protocolVersion() == Rpc::ProtocolVersion::ChainPack)? Utils::toHex(packed_data, 0, 250): packed_data.substr(0, 250));
	enqueueDataToSend(Chunk{std::move(packed_data)});
}

void RpcDriver::sendRawData(std::string &&data)
{
	logRpcMsg() << "send raw data: " << (data.size() > 250? "<... long data ...>" : Utils::toHex(data));
	enqueueDataToSend(Chunk{std::move(data)});
}

void RpcDriver::sendRawData(RpcValue::MetaData &&meta_data, std::string &&data)
{
	logRpcMsg() << "protocol:" << Rpc::ProtocolVersionToString(protocolVersion()) << "send raw meta + data: " << meta_data.toStdString()
				<< Utils::toHex(data, 0, 250);
	using namespace std;
	//shvLogFuncFrame() << msg.toStdString();
	std::ostringstream os_packed_meta_data;
	switch (protocolVersion()) {
	case Rpc::ProtocolVersion::Cpon: {
		CponWriter wr(os_packed_meta_data);
		wr << meta_data;
		break;
	}
	case Rpc::ProtocolVersion::ChainPack: {
		ChainPackWriter wr(os_packed_meta_data);
		wr << meta_data;
		break;
	}
	default:
		SHVCHP_EXCEPTION("Cannot serialize data without protocol version specified.")
	}
	Rpc::ProtocolVersion packed_data_ver = RpcMessage::protocolVersion(meta_data);
	if(packed_data_ver == protocolVersion()) {
		enqueueDataToSend(Chunk(os_packed_meta_data.str(), std::move(data)));
	}
	else if(packed_data_ver != Rpc::ProtocolVersion::Invalid) {
		// recode data;
		RpcValue val = decodeData(packed_data_ver, data, 0);
		enqueueDataToSend(Chunk(os_packed_meta_data.str(), codeRpcValue(protocolVersion(), val)));
	}
	else {
		SHVCHP_EXCEPTION("Cannot decode data without protocol version specified.")
	}
}

RpcMessage RpcDriver::composeRpcMessage(RpcValue::MetaData &&meta_data, const std::string &data, bool throw_exc)
{
	Rpc::ProtocolVersion packed_data_ver = RpcMessage::protocolVersion(meta_data);
	RpcValue val = decodeData(packed_data_ver, data, 0);
	if(!val.isValid()) {
		const char * msg = "Compose RPC message error.";
		if(throw_exc) {
			SHVCHP_EXCEPTION(msg);
		}
		else {
			nError() << msg;
			return RpcMessage();
		}
	}
	val.setMetaData(std::move(meta_data));
	return RpcMessage(val);
}

void RpcDriver::enqueueDataToSend(RpcDriver::Chunk &&chunk_to_enqueue)
{
	/// LOCK_FOR_SEND lock mutex here in the multithreaded environment
	lockSendQueue();
	if(!chunk_to_enqueue.empty())
		m_chunkQueue.push_back(std::move(chunk_to_enqueue));
	if(!isOpen()) {
		nError() << "write data error, socket is not open!";
		return;
	}
	flush();
	writeQueue();
	/// UNLOCK_FOR_SEND unlock mutex here in the multithreaded environment
	unlockSendQueue();
}

void RpcDriver::writeQueue()
{
	if(m_chunkQueue.empty())
		return;
	logRpcData() << "writePendingData(), queue len:" << m_chunkQueue.size();
	//static int hi_cnt = 0;
	const Chunk &chunk = m_chunkQueue[0];

	if(!m_topChunkHeaderWritten) {
		std::string protocol_version_data;
		{
			std::ostringstream os;
			ChainPackWriter wr(os);
			wr.writeUIntData((unsigned)protocolVersion());
			protocol_version_data = os.str();
		}
		{
			std::ostringstream os;
			ChainPackWriter wr(os);
			wr.writeUIntData(chunk.size() + protocol_version_data.length());
			std::string packet_len_data = os.str();
			auto len = writeBytes(packet_len_data.data(), packet_len_data.length());
			if(len < 0)
				SHVCHP_EXCEPTION("Write socket error!");
			if(len < (int)packet_len_data.length())
				SHVCHP_EXCEPTION("Design error! Chunk length shall be always written at once to the socket");
		}
		{
			auto len = writeBytes(protocol_version_data.data(), protocol_version_data.length());
			if(len < 0)
				SHVCHP_EXCEPTION("Write socket error!");
			if(len != 1)
				SHVCHP_EXCEPTION("Design error! Protocol version shall be always written at once to the socket");
		}
		m_topChunkHeaderWritten = true;
	}
	if(m_topChunkBytesWrittenSoFar < chunk.metaData.size()) {
		m_topChunkBytesWrittenSoFar += writeBytes_helper(chunk.metaData, m_topChunkBytesWrittenSoFar, chunk.metaData.size() - m_topChunkBytesWrittenSoFar);
	}
	if(m_topChunkBytesWrittenSoFar >= chunk.metaData.size()) {
		m_topChunkBytesWrittenSoFar += writeBytes_helper(chunk.data
														 , m_topChunkBytesWrittenSoFar - chunk.metaData.size()
														 , chunk.data.size() - (m_topChunkBytesWrittenSoFar - chunk.metaData.size()));
		//logRpc() << "writeQueue - data len:" << chunk.length() << "start index:" << m_topChunkBytesWrittenSoFar << "bytes written:" << len << "remaining:" << (chunk.length() - m_topChunkBytesWrittenSoFar - len);
	}
	if(m_topChunkBytesWrittenSoFar == chunk.size()) {
		m_topChunkHeaderWritten = false;
		m_topChunkBytesWrittenSoFar = 0;
		m_chunkQueue.pop_front();
	}
}

int64_t RpcDriver::writeBytes_helper(const std::string &str, size_t from, size_t length)
{
	auto len = writeBytes(str.data() + from, length);
	if(len < 0)
		SHVCHP_EXCEPTION("Write socket error!");
	if(len == 0)
		SHVCHP_EXCEPTION("Design error! At least 1 byte of data shall be always written to the socket");
	return len;
}

void RpcDriver::onBytesRead(std::string &&bytes)
{
	logRpcData() << bytes.length() << "bytes of data read";
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
	logRpcData() << __FUNCTION__ << "data len:" << read_data.length();

	using namespace shv::chainpack;

	std::istringstream in(read_data);

	bool ok;
	uint64_t chunk_len = ChainPackReader::readUIntData(in, &ok);
	if(!ok)
		return 0;

	size_t read_len = (size_t)in.tellg() + chunk_len;

	Rpc::ProtocolVersion protocol_version = (Rpc::ProtocolVersion)ChainPackReader::readUIntData(in, &ok);
	if(!ok)
		return 0;

	logRpcData() << "\t chunk len:" << chunk_len << "read_len:" << read_len << "stream pos:" << in.tellg();
	if(read_len > read_data.length())
		return 0;

	if(in.tellg() < 0)
		return 0;

	if(m_protocolVersion == Rpc::ProtocolVersion::Invalid && protocol_version != Rpc::ProtocolVersion::Invalid) {
		// if protocol version is not explicitly specified,
		// it is set from first received message (should be knockknock)
		m_protocolVersion = protocol_version;
	}

	RpcValue::MetaData meta_data;
	size_t meta_data_end_pos = decodeMetaData(meta_data, protocol_version, read_data, in.tellg());
	onRpcDataReceived(protocol_version, std::move(meta_data), read_data, meta_data_end_pos, read_len - meta_data_end_pos);

	return read_len;
}

size_t RpcDriver::decodeMetaData(RpcValue::MetaData &meta_data, Rpc::ProtocolVersion protocol_version, const std::string &data, size_t start_pos)
{
	size_t meta_data_end_pos = start_pos;
	std::istringstream in(data);
	in.seekg(start_pos);
	try {
		switch (protocol_version) {
		case Rpc::ProtocolVersion::JsonRpc: {
			CponReader rd(in);
			RpcValue msg;
			rd.read(msg);
			if(!msg.isMap()) {
				nError() << "JSON message cannot be translated to ChainPack";
			}
			else {
				const RpcValue::Map &map = msg.toMap();
				unsigned id = map.value(Rpc::JSONRPC_ID).toUInt();
				unsigned caller_id = map.value(Rpc::JSONRPC_CALLER_ID).toUInt();
				RpcValue::String method = map.value(Rpc::JSONRPC_METHOD).toString();
				std::string shv_path = map.value(Rpc::JSONRPC_SHV_PATH).toString();
				if(id > 0)
					RpcMessage::setRequestId(meta_data, id);
				if(!method.empty())
					RpcMessage::setMethod(meta_data, method);
				if(!shv_path.empty())
					RpcMessage::setShvPath(meta_data, shv_path);
				if(caller_id > 0)
					RpcMessage::setCallerId(meta_data, caller_id);
			}
			break;
		}
		case Rpc::ProtocolVersion::Cpon: {
			CponReader rd(in);
			rd.read(meta_data);
			meta_data_end_pos = (in.tellg() < 0)? data.size(): (size_t)in.tellg();
			break;
		}
		case Rpc::ProtocolVersion::ChainPack: {
			ChainPackReader rd(in);
			rd.read(meta_data);
			meta_data_end_pos = (in.tellg() < 0)? data.size(): (size_t)in.tellg();
			break;
		}
		default:
			meta_data_end_pos = data.size();
			nError() << "Throwing away message with unknown protocol version:" << (unsigned)protocol_version;
			break;
		}
	}
	catch(CponReader::ParseException &e) {
		nError() << e.mesage();
	}
	return meta_data_end_pos;
}

RpcValue RpcDriver::decodeData(Rpc::ProtocolVersion protocol_version, const std::string &data, size_t start_pos)
{
	RpcValue ret;
	std::istringstream in(data);
	in.seekg(start_pos);
	try {
		switch (protocol_version) {
		case Rpc::ProtocolVersion::JsonRpc: {
			CponReader rd(in);
			rd.read(ret);
			RpcValue::Map map = ret.toMap();
			map.erase(Rpc::JSONRPC_ID);
			map.erase(Rpc::JSONRPC_CALLER_ID);
			map.erase(Rpc::JSONRPC_METHOD);
			map.erase(Rpc::JSONRPC_SHV_PATH);
			break;
		}
		case Rpc::ProtocolVersion::Cpon: {
			CponReader rd(in);
			rd.read(ret);
			break;
		}
		case Rpc::ProtocolVersion::ChainPack: {
			ChainPackReader rd(in);
			rd.read(ret);
			break;
		}
		default:
			nError() << "Don't know how to decode message with unknown protocol version:" << (unsigned)protocol_version;
			break;
		}
	}
	catch(CponReader::ParseException &e) {
		nError() << e.mesage();
	}
	return ret;
}

std::string RpcDriver::codeRpcValue(Rpc::ProtocolVersion protocol_version, const RpcValue &val)
{
	std::ostringstream os_packed_data;
	switch (protocol_version) {
	case Rpc::ProtocolVersion::JsonRpc: {
		RpcValue::Map json_msg;
		RpcMessage rpc_msg(msg);

		const RpcValue::UInt rq_id = rpc_msg.requestId();
		if(rq_id > 0)
			json_msg[Rpc::JSONRPC_ID] = rq_id;

		const RpcValue::String shv_path = rpc_msg.shvPath();
		if(!shv_path.empty())
			json_msg[Rpc::JSONRPC_SHV_PATH] = shv_path;

		const RpcValue::UInt caller_id = rpc_msg.callerId();
		if(caller_id > 0)
			json_msg[Rpc::JSONRPC_CALLER_ID] = caller_id;
					;
		if(rpc_msg.isResponse()) {
			// response
			RpcResponse resp(rpc_msg);
			if(resp.isError())
				json_msg[Rpc::JSONRPC_ERROR] = resp.error();
			else
				json_msg[Rpc::JSONRPC_RESULT] = resp.result();
		}
		else {
			// request
			RpcRequest rq(rpc_msg);
			json_msg[Rpc::JSONRPC_METHOD] = rq.method();
			json_msg[Rpc::JSONRPC_PARAMS] = rq.params();
		}
		CponWriter wr(os_packed_data);
		wr.write(json_msg);
		break;
	}
	case Rpc::ProtocolVersion::Cpon: {
		CponWriter wr(os_packed_data);
		wr << val;
		break;
	}
	case Rpc::ProtocolVersion::ChainPack: {
		ChainPackWriter wr(os_packed_data);
		wr << val;
		break;
	}
	default:
		SHVCHP_EXCEPTION("Cannot serialize data without protocol version specified.")
	}
	return os_packed_data.str();
}

void RpcDriver::onRpcDataReceived(Rpc::ProtocolVersion protocol_version, RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	(void)data_len;
	RpcValue msg = decodeData(protocol_version, data, start_pos);
	if(msg.isValid()) {
		logRpcMsg() << RCV_LOG_ARROW << msg.toPrettyString();
		msg.setMetaData(std::move(md));
		onRpcValueReceived(msg);
	}
	else {
		nError() << "Throwing away message with unknown protocol version:" << (unsigned)protocol_version;
	}
}

void RpcDriver::onRpcValueReceived(const RpcValue &msg)
{
	logRpcData() << "\t message received:" << msg.toCpon();
	//logLongFiles() << "\t emitting message received:" << msg.dumpText();
	if(m_messageReceivedCallback)
		m_messageReceivedCallback(msg);
}

} // namespace chainpack
} // namespace shv
