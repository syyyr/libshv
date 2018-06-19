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

#define logRpcRawMsg() nCDebug("RpcRawMsg")
#define logRpcData() nCDebug("RpcData")
//#define logRpcData() nInfo()

//#define logRpcSyncCalls() nCDebug("RpcSyncCalls")

namespace shv {
namespace chainpack {

const char * RpcDriver::SND_LOG_ARROW = "==>";
const char * RpcDriver::RCV_LOG_ARROW = "<==";

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
	logRpcRawMsg() << SND_LOG_ARROW << msg.toPrettyString();
	std::string packed_data = codeRpcValue(protocolType(), msg);
	logRpcData() << "protocol:" << Rpc::protocolTypeToString(protocolType())
				 << "packed data:"
				 << ((protocolType() == Rpc::ProtocolType::ChainPack)? Utils::toHex(packed_data, 0, 250): packed_data.substr(0, 250));
	enqueueDataToSend(Chunk{std::move(packed_data)});
}

void RpcDriver::sendRawData(std::string &&data)
{
	logRpcRawMsg() << SND_LOG_ARROW << "send raw data: " << (data.size() > 250? "<... long data ...>" : Utils::toHex(data));
	enqueueDataToSend(Chunk{std::move(data)});
}

void RpcDriver::sendRawData(const RpcValue::MetaData &meta_data, std::string &&data)
{
	logRpcRawMsg() << SND_LOG_ARROW << "protocol:" << Rpc::protocolTypeToString(protocolType()) << "send raw meta + data: " << meta_data.toStdString()
				<< Utils::toHex(data, 0, 250);
	using namespace std;
	//shvLogFuncFrame() << msg.toStdString();
	std::ostringstream os_packed_meta_data;
	switch (protocolType()) {
	case Rpc::ProtocolType::Cpon: {
		CponWriter wr(os_packed_meta_data);
		wr << meta_data;
		break;
	}
	case Rpc::ProtocolType::ChainPack: {
		ChainPackWriter wr(os_packed_meta_data);
		wr << meta_data;
		break;
	}
	case Rpc::ProtocolType::JsonRpc: {
		break;
	}
	default:
		SHVCHP_EXCEPTION("Cannot serialize data without protocol version specified.")
	}
	Rpc::ProtocolType packed_data_ver = RpcMessage::protocolType(meta_data);
	if(protocolType() == Rpc::ProtocolType::JsonRpc) {
		// JSON RPC must be handled separately
		if(packed_data_ver == Rpc::ProtocolType::Invalid)
			SHVCHP_EXCEPTION("Cannot serialize to JSON-RPC data without protocol version specified.")
		// recode data;
		RpcValue val = decodeData(packed_data_ver, data, 0);
		val.setMetaData(RpcValue::MetaData(meta_data));
		enqueueDataToSend(Chunk(codeRpcValue(Rpc::ProtocolType::JsonRpc, val)));
	}
	else {
		if(packed_data_ver == Rpc::ProtocolType::Invalid || packed_data_ver == protocolType()) {
			enqueueDataToSend(Chunk(os_packed_meta_data.str(), std::move(data)));
		}
		else {
			// recode data;
			RpcValue val = decodeData(packed_data_ver, data, 0);
			enqueueDataToSend(Chunk(os_packed_meta_data.str(), codeRpcValue(protocolType(), val)));
		}
	}
}

RpcMessage RpcDriver::composeRpcMessage(RpcValue::MetaData &&meta_data, const std::string &data, std::string *errmsg)
{
	Rpc::ProtocolType protocol_type = RpcMessage::protocolType(meta_data);
	RpcValue val = decodeData(protocol_type, data, 0);
	if(!val.isValid()) {
		const char * msg = "Compose RPC message error.";
		if(!errmsg) {
			SHVCHP_EXCEPTION(msg);
		}
		else {
			*errmsg = std::move(msg);
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
	if(!chunk_to_enqueue.empty()) {
		m_chunkQueue.push_back(std::move(chunk_to_enqueue));
	}
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
		std::string protocol_type_data;
		{
			std::ostringstream os;
			ChainPackWriter wr(os);
			wr.writeUIntData((unsigned)protocolType());
			protocol_type_data = os.str();
		}
		{
			std::ostringstream os;
			ChainPackWriter wr(os);
			wr.writeUIntData(chunk.size() + protocol_type_data.length());
			std::string packet_len_data = os.str();
			auto len = writeBytes(packet_len_data.data(), packet_len_data.length());
			if(len < 0)
				SHVCHP_EXCEPTION("Write socket error!");
			if(len < (int)packet_len_data.length())
				SHVCHP_EXCEPTION("Design error! Chunk length shall be always written at once to the socket");
		}
		{
			auto len = writeBytes(protocol_type_data.data(), protocol_type_data.length());
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
	logRpcData().nospace() << __FUNCTION__ << " " << bytes.length() << " bytes of data read:\n" << shv::chainpack::Utils::hexDump(bytes);
	m_readData += std::string(std::move(bytes));
	while(true) {
		int len = processReadData(m_readData);
		logRpcData() << len << "bytes of" << m_readData.size() << "processed";
		if(len > 0) {
			//nWarning().nospace() << "1:" << m_readData.size() << " '" << m_readData << "'";
			m_readData = m_readData.substr(len);
			//nWarning().nospace() << "2:" << m_readData.size() << " '" << m_readData << "'";
		}
		else {
			break;
		}
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

	Rpc::ProtocolType protocol_type = (Rpc::ProtocolType)ChainPackReader::readUIntData(in, &ok);
	if(!ok)
		return 0;

	logRpcData() << "\t expected message data length:" << read_len << "length available:" << read_data.size();
	if(read_len > read_data.length())
		return 0;

	if(in.tellg() < 0)
		return 0;

	if(m_protocolType == Rpc::ProtocolType::Invalid && protocol_type != Rpc::ProtocolType::Invalid) {
		// if protocol version is not explicitly specified,
		// it is set from first received message
		m_protocolType = protocol_type;
	}

	try {
		RpcValue::MetaData meta_data;
		size_t meta_data_end_pos = decodeMetaData(meta_data, protocol_type, read_data, in.tellg());
		onRpcDataReceived(protocol_type, std::move(meta_data), read_data, meta_data_end_pos, read_len - meta_data_end_pos);
	}
	catch (std::exception &e) {
		nError() << "processReadData error:" << e.what();
		onProcessReadDataException(e);
	}

	return read_len;
}

size_t RpcDriver::decodeMetaData(RpcValue::MetaData &meta_data, Rpc::ProtocolType protocol_type, const std::string &data, size_t start_pos)
{
	size_t meta_data_end_pos = start_pos;
	std::istringstream in(data);
	in.seekg(start_pos);

	switch (protocol_type) {
	case Rpc::ProtocolType::JsonRpc: {
		CponReader rd(in);
		RpcValue msg;
		rd.read(msg);
		if(!msg.isMap()) {
			nError() << "JSON message cannot be translated to ChainPack";
		}
		else {
			const RpcValue::Map &map = msg.toMap();
			unsigned id = map.value(Rpc::JSONRPC_REQUEST_ID).toUInt();
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
				RpcMessage::setCallerIds(meta_data, caller_id);
		}
		break;
	}
	case Rpc::ProtocolType::Cpon: {
		CponReader rd(in);
		rd.read(meta_data);
		meta_data_end_pos = (in.tellg() < 0)? data.size(): (size_t)in.tellg();
		break;
	}
	case Rpc::ProtocolType::ChainPack: {
		ChainPackReader rd(in);
		rd.read(meta_data);
		meta_data_end_pos = (in.tellg() < 0)? data.size(): (size_t)in.tellg();
		break;
	}
	default:
		SHVCHP_EXCEPTION("Unknown protocol type: " + Utils::toString((int)protocol_type));
		//meta_data_end_pos = data.size();
		//nError() << "Throwing away message with unknown protocol version:" << (unsigned)protocol_type;
		break;
	}

	return meta_data_end_pos;
}

RpcValue RpcDriver::decodeData(Rpc::ProtocolType protocol_type, const std::string &data, size_t start_pos)
{
	RpcValue ret;
	std::istringstream in(data);
	in.seekg(start_pos);
	try {
		switch (protocol_type) {
		case Rpc::ProtocolType::JsonRpc: {
			CponReader rd(in);
			rd.read(ret);
			RpcValue::Map map = ret.toMap();
			RpcValue::IMap imap;
			RpcValue params = map.value(Rpc::JSONRPC_PARAMS);
			if(params.isValid()) {
				imap[RpcMessage::MetaType::Key::Params] = params;
			}
			else {
				RpcValue result = map.value(Rpc::JSONRPC_RESULT);
				if(result.isValid()) {
					imap[RpcMessage::MetaType::Key::Result] = result;
				}
				else {
					RpcValue error = map.value(Rpc::JSONRPC_ERROR);
					if(error.isValid())
						imap[RpcMessage::MetaType::Key::Error] = RpcResponse::Error::fromJson(error.toMap());
				}
			}
			ret = imap;
			break;
		}
		case Rpc::ProtocolType::Cpon: {
			CponReader rd(in);
			rd.read(ret);
			break;
		}
		case Rpc::ProtocolType::ChainPack: {
			ChainPackReader rd(in);
			rd.read(ret);
			break;
		}
		default:
			nError() << "Don't know how to decode message with unknown protocol version:" << (unsigned)protocol_type;
			break;
		}
	}
	catch(CponReader::ParseException &e) {
		nError() << e.what();
	}
	return ret;
}

std::string RpcDriver::codeRpcValue(Rpc::ProtocolType protocol_type, const RpcValue &val)
{
	std::ostringstream os_packed_data;
	switch (protocol_type) {
	case Rpc::ProtocolType::JsonRpc: {
		RpcValue::Map json_msg;
		RpcMessage rpc_msg(val);

		const RpcValue rq_id = rpc_msg.requestId();
		if(rq_id.isValid())
			json_msg[Rpc::JSONRPC_REQUEST_ID] = rq_id;

		const RpcValue shv_path = rpc_msg.shvPath();
		if(shv_path.isString())
			json_msg[Rpc::JSONRPC_SHV_PATH] = shv_path.toString();

		const RpcValue caller_id = rpc_msg.callerIds();
		if(caller_id.isValid())
			json_msg[Rpc::JSONRPC_CALLER_ID] = caller_id;
					;
		if(rpc_msg.isResponse()) {
			// response
			RpcResponse resp(rpc_msg);
			if(resp.isError())
				json_msg[Rpc::JSONRPC_ERROR] = resp.error().toJson();
			else
				json_msg[Rpc::JSONRPC_RESULT] = resp.result();
		}
		else {
			// request
			RpcRequest rq(rpc_msg);
			json_msg[Rpc::JSONRPC_METHOD] = rq.method();
			if(rq.params().isValid())
				json_msg[Rpc::JSONRPC_PARAMS] = rq.params();
		}
		CponWriterOptions opts;
		opts.setJsonFormat(true);
		CponWriter wr(os_packed_data, opts);
		wr.write(json_msg);
		break;
	}
	case Rpc::ProtocolType::Cpon: {
		CponWriter wr(os_packed_data);
		wr << val;
		break;
	}
	case Rpc::ProtocolType::ChainPack: {
		ChainPackWriter wr(os_packed_data);
		wr << val;
		break;
	}
	default:
		SHVCHP_EXCEPTION("Cannot serialize data without protocol version specified.")
	}
	return os_packed_data.str();
}

void RpcDriver::onRpcDataReceived(Rpc::ProtocolType protocol_type, RpcValue::MetaData &&md, const std::string &data, size_t start_pos, size_t data_len)
{
	//nInfo() << __FILE__ << RCV_LOG_ARROW << md.toStdString() << shv::chainpack::Utils::toHexElided(data, start_pos, 100);
	(void)data_len;
	RpcValue msg = decodeData(protocol_type, data, start_pos);
	if(msg.isValid()) {
		msg.setMetaData(std::move(md));
		logRpcRawMsg() << RCV_LOG_ARROW << msg.toPrettyString();
		onRpcValueReceived(msg);
	}
	else {
		nError() << "Throwing away message with unknown protocol version:" << (unsigned)protocol_type;
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
