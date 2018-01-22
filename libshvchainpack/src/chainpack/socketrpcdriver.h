#pragma once

#include "rpcdriver.h"

#include <string>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT SocketRpcDriver : public RpcDriver
{
	using Super = RpcDriver;
public:
	SocketRpcDriver();
	~SocketRpcDriver() override;
	virtual bool connectToHost(const std::string & host, int port);
	virtual void closeConnection();
	void exec();

	void sendNotify(std::string &&method, const RpcValue &result);
protected:
	bool isOpen() override;
	int64_t writeBytes(const char *bytes, size_t length) override;
	bool flush() override;

	void sendResponse(int request_id, const RpcValue &result);

	virtual void idleTaskOnSelectTimeout() {}
	//virtual void connectedToHost(bool ) {}
	//virtual void connectionClosed() {}
private:
	int m_socket = -1;
	std::string m_writeBuffer;
	size_t m_maxWriteBufferLength = 1024;
};

}}
