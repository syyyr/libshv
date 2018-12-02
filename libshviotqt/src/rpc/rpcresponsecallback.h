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

class SHVIOTQT_DECL_EXPORT RpcResponseCallBack : public QObject
{
	Q_OBJECT
public:
	using CallBackFunction = std::function<void (const shv::chainpack::RpcResponse &rsp)>;

	SHV_FIELD_IMPL(int, r, R, equestId)
	SHV_FIELD_IMPL2(int, t, T, imeout, 30*60*1000)

public:
	explicit RpcResponseCallBack(int rq_id, QObject *parent = nullptr);
	void start(CallBackFunction cb);
	virtual void onRpcMessageReceived(const shv::chainpack::RpcMessage &msg);
private:
	CallBackFunction m_callBackFunction;
	QTimer *m_timeoutTimer = nullptr;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
