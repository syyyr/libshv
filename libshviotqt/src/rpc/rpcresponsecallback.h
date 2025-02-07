#pragma once

#include "../shviotqtglobal.h"

#include <shv/core/utils.h>
#include <shv/chainpack/rpc.h>
#include <shv/chainpack/rpcvalue.h>

#include <QObject>
#include <QPointer>

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
	bool m_isFinished = false;
};


class SHVIOTQT_DECL_EXPORT RpcCall : public QObject
{
	Q_OBJECT
public:
	static RpcCall* createSubscriptionRequest(::shv::iotqt::rpc::ClientConnection *connection, const QString &shv_path, const QString &method = QString(shv::chainpack::Rpc::SIG_VAL_CHANGED));
	static RpcCall* create(::shv::iotqt::rpc::ClientConnection *connection);
	RpcCall* setShvPath(const std::string &shv_path);
	RpcCall* setShvPath(const char *shv_path);
	RpcCall* setShvPath(const QString &shv_path);
	RpcCall* setShvPath(const QStringList &shv_path);
	RpcCall* setMethod(const std::string &method);
	RpcCall* setMethod(const char *method);
	RpcCall* setMethod(const QString &method);
	RpcCall* setParams(const ::shv::chainpack::RpcValue &params);
	RpcCall* setTimeout(int timeout) { m_timeout = timeout; return this; }
	//RpcCall* setParams(const QVariant &params); not implemented since we do not know how to convert arbitrary QVariant to RpcValue
	RpcCall* setUserId(const ::shv::chainpack::RpcValue &user_id);

	void start();

	Q_SIGNAL void maybeResult(const ::shv::chainpack::RpcValue &result, const QString &error);
	Q_SIGNAL void result(const ::shv::chainpack::RpcValue &result);
	Q_SIGNAL void error(const QString &error);
private:
	RpcCall(::shv::iotqt::rpc::ClientConnection *connection);
private:
	QPointer<::shv::iotqt::rpc::ClientConnection> m_rpcConnection;
	std::string m_shvPath;
	std::string m_method;
	shv::chainpack::RpcValue m_params;
	int m_timeout = 0;
	shv::chainpack::RpcValue m_userId;
};

} // namespace rpc
} // namespace iotqt
} // namespace shv
