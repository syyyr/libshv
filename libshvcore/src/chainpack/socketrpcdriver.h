#pragma once

#include "rpcdriver.h"

#include <string>

namespace shv {
namespace core {
namespace chainpack {

class SocketRpcDriver : public shv::core::chainpack::RpcDriver
{
	using Super = shv::core::chainpack::RpcDriver;
public:
	SocketRpcDriver();
	~SocketRpcDriver() override;
	virtual bool connectToHost(const std::string & host, int port);
	virtual void closeConnection();
	void exec();

	void sendNotify(std::string &&method, const shv::core::chainpack::RpcValue &result);
protected:
	bool isOpen() override;
	size_t bytesToWrite() override;
	int64_t writeBytes(const char *bytes, size_t length) override;
	bool flushNoBlock() override;
	void sendResponse(int request_id, const shv::core::chainpack::RpcValue &result);

	virtual void idleTaskOnSelectTimeout() {}
	//virtual void connectedToHost(bool ) {}
	//virtual void connectionClosed() {}
private:
	int m_socket = -1;
	std::string m_bytesToWrite;
	size_t m_maxBytesToWriteLength = 4 * 1024;
};

}}}
