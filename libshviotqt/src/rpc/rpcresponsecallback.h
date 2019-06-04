#pragma once

#include "../shviotqtglobal.h"

#include <shv/core/utils.h>

#include <QObject>

#include <functional>

class QTimer;

namespace shv {

namespace chainpack { class RpcMessage; class RpcResponse; }

namespace iotqt {
namespace rpc {

class ClientConnection;

class SHVIOTQT_DECL_EXPORT RpcResponseCallBack : public QObject
{
	Q_OBJECT
public:
	using CallBackFunction = std::function<void (const shv::chainpack::RpcResponse &rsp)>;

	SHV_FIELD_IMPL(int, r, R, equestId)
	SHV_FIELD_IMPL2(int, t, T, imeout, 1*60*1000)

public:
	explicit RpcResponseCallBack(int rq_id, QObject *parent = nullptr);
	explicit RpcResponseCallBack(shv::iotqt::rpc::ClientConnection *conn, int rq_id, QObject *parent = nullptr);

	Q_SIGNAL void finished(const shv::chainpack::RpcResponse &response);

	void start();
	void start(int time_out);
	void start(CallBackFunction cb);
	void start(int time_out, CallBackFunction cb);
	void start(QObject *context, CallBackFunction cb);
	void start(int time_out_msec, QObject *context, CallBackFunction cb);
	void abort();
	virtual void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
private:
	CallBackFunction m_callBackFunction;
	QTimer *m_timeoutTimer = nullptr;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
