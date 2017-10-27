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
	bool connectToHost(const std::string & host, int port);
	void closeConnection();
	void exec();
protected:
	bool isOpen() override;
	size_t bytesToWrite() override;
	int64_t writeBytes(const char *bytes, size_t length) override;
	int64_t flushNoBlock() override;
	void sendResponse(int request_id, const shv::core::chainpack::RpcValue &result);
	void sendNotify(std::string &&method, const shv::core::chainpack::RpcValue &result);

	virtual void idleTaskOnSelectTimeout() {}
private:
	int m_socket = -1;
	std::string m_bytesToWrite;
};

}}}
