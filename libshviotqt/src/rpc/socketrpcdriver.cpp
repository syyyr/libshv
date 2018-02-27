#include "socketrpcdriver.h"
#include "rpc.h"

#include <shv/coreqt/log.h>

#include <shv/core/exception.h>
#include <shv/chainpack/rpcmessage.h>

#include <QTimer>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QTcpSocket>
#include <QHostAddress>

//#define DUMP_DATA_FILE

#ifdef DUMP_DATA_FILE
#include <QFile>
#endif


#define smcDebug QNoDebug

#define logRpcMsg() shvCDebug("RpcMsg")
#define logRpcData() shvCDebug("RpcData")
#define logRpcSyncCalls() shvCDebug("RpcSyncCalls")

namespace cp = shv::chainpack;
//namespace cpq = shv::iotqt::rpc;

namespace shv {
namespace iotqt {
namespace rpc {

static int s_connectionId = 0;

SocketRpcDriver::SocketRpcDriver(QObject *parent)
	: QObject(parent)
	, m_connectionId(++s_connectionId)
{
	Rpc::registerMetatTypes();
	/*
	setMessageReceivedCallback([this](const shv::shv::chainpack::RpcValue &msg) {
		emit messageReceived(msg);
	});
	*/
#ifdef DUMP_DATA_FILE
	QFile *f = new QFile("/tmp/rpc.dat", this);
	f->setObjectName("DUMP_DATA_FILE");
	if(!f->open(QFile::WriteOnly)) {
		shvError() << "cannot open file" << f->fileName() << "for write";
		delete f;
	}
	shvInfo() << "Dumping RPC stream to file:" << f->fileName();
#endif
}

SocketRpcDriver::~SocketRpcDriver()
{
	shvDebug() << __FUNCTION__;
	abortConnection();
}

void SocketRpcDriver::setSocket(QTcpSocket *socket)
{
	socket->moveToThread(this->thread());
	m_socket = socket;
	connect(socket, &QTcpSocket::readyRead, this, &SocketRpcDriver::onReadyRead);
	connect(socket, &QTcpSocket::bytesWritten, this, &SocketRpcDriver::onBytesWritten, Qt::QueuedConnection);
	connect(socket, &QTcpSocket::connected, [this]() {
		shvDebug() << this << "Socket connected!!!";
		//shvWarning() << (peerAddress().toStdString() + ':' + std::to_string(peerPort()));
		setSocketConnected(true);
	});
	connect(socket, &QTcpSocket::disconnected, [this]() {
		shvDebug() << this << "Socket disconnected!!!";
		m_isSocketConnected = false;
		setSocketConnected(false);
	});
}

QTcpSocket *SocketRpcDriver::socket()
{
	if(!m_socket)
		SHV_EXCEPTION("Socket is NULL!");
	return m_socket;
}

bool SocketRpcDriver::isSocketConnected() const
{
	return m_socket && m_isSocketConnected;
}

void SocketRpcDriver::setSocketConnected(bool b)
{
	if(b != m_isSocketConnected) {
		m_isSocketConnected = b;
		emit socketConnectedChanged(b);
	}
}

void SocketRpcDriver::connectToHost(const QString &host_name, quint16 port)
{
	socket()->connectToHost(host_name, port);
}

void SocketRpcDriver::onReadyRead()
{
	QByteArray ba = socket()->readAll();
#ifdef DUMP_DATA_FILE
	QFile *f = findChild<QFile*>("DUMP_DATA_FILE");
	if(f) {
		f->write(ba.constData(), ba.length());
		f->flush();
	}
#endif
	onBytesRead(ba.toStdString());
}

void SocketRpcDriver::onBytesWritten()
{
	logRpcData() << "onBytesWritten()";
	enqueueDataToSend(Chunk());
}

bool SocketRpcDriver::isOpen()
{
	return m_socket && m_socket->isOpen();
}

int64_t SocketRpcDriver::writeBytes(const char *bytes, size_t length)
{
	return socket()->write(bytes, length);
}

bool SocketRpcDriver::flush()
{
	if(m_socket)
		return m_socket->flush();
	return false;
}

void SocketRpcDriver::onRpcValueReceived(const shv::chainpack::RpcValue &msg)
{
	emit rpcMessageReceived(msg);
}

namespace {
class ConnectionScope
{
public:
	explicit ConnectionScope(QMetaObject::Connection &c) : m_connection(c) {}
	~ConnectionScope() {if(m_connection) QObject::disconnect(m_connection);}
private:
	QMetaObject::Connection &m_connection;
};
}

void SocketRpcDriver:: sendRequestQuasiSync(const shv::chainpack::RpcRequest &request, shv::chainpack::RpcResponse *presponse, int time_out_ms)
{
	namespace cp = shv::chainpack;
	smcDebug() << Q_FUNC_INFO << "timeout ms:" << time_out_ms;
	if(time_out_ms == 0) {
		smcDebug() << "sendMessageSync called with invalid timeout:" << time_out_ms << "," << defaultRpcTimeout() << "msec will be used instead.";
		time_out_ms = defaultRpcTimeout();
	}
	shv::chainpack::RpcResponse resp_msg;
	do {
		cp::RpcValue::UInt msg_id = request.id();
		if(msg_id == 0) {
			shvWarning() << "Attempt to send RPC request with ID not set, message will be ignored";
			break;
		}
		smcDebug() << "sending message:" << request.toCpon();
		logRpcSyncCalls() << SND_LOG_ARROW << "SEND SYNC message id:" << msg_id << "msg:" << request.toCpon();
		sendMessage(request.value());
		QElapsedTimer tm_elapsed;
		tm_elapsed.start();
		QEventLoop eloop;
		QMetaObject::Connection lambda_connection;
		lambda_connection = connect(this, &SocketRpcDriver::rpcMessageReceived, [&eloop, &resp_msg, &lambda_connection, msg_id](const shv::chainpack::RpcValue &msg_val)
		{
			shv::chainpack::RpcMessage msg(msg_val);
			smcDebug() << &eloop << "New RPC message id:" << msg.id();
			if(msg.id() == msg_id) {
				QObject::disconnect(lambda_connection);
				lambda_connection = QMetaObject::Connection();
				resp_msg = msg;
				smcDebug() << "\tID MATCH stopping event loop ..." << &eloop;
				eloop.quit();
			}
			else if(msg.id() == 0) {
				//logRpcSyncCalls() << "<=== RECEIVE NOTIFY while waiting for SYNC response:" << msg.jsonRpcMessage().toString();
			}
			else {
				logRpcSyncCalls() << "RECV other message while waiting for SYNC response id:" << msg.id() << "json:" << msg.toCpon();
			}
		});
		ConnectionScope cscp(lambda_connection);
		if(time_out_ms > 0)
			QTimer::singleShot(time_out_ms, &eloop, &QEventLoop::quit);

		smcDebug() << "\t entering event loop ..." << &eloop;
		eloop.exec();
		smcDebug() << "\t event loop" << &eloop << "exec exit, message received:" << resp_msg.isValid();
		logRpcSyncCalls() << RCV_LOG_ARROW << "RECV SYNC message id:" << resp_msg.id() << "msg:" << resp_msg.toCpon();
		int elapsed = (int)tm_elapsed.elapsed();
		if(elapsed >= time_out_ms) {
			cp::RpcValue::String err_msg = "Receive message timeout after: " + std::to_string(elapsed) + " msec!";
			shvError() << err_msg;
			auto err = cp::RpcResponse::Error::createInternalError(err_msg);
			resp_msg.setId(msg_id);
			resp_msg.setError(err);
		}
		if(!resp_msg.isValid()) {
			cp::RpcValue::String err_msg = "Invalid mesage returned from sync call!";
			shvError() << err_msg;

			auto err = cp::RpcResponse::Error::createInternalError(err_msg);
			resp_msg.setId(msg_id);
			resp_msg.setError(err);
		}
	} while(false);
	if(presponse)
		*presponse = resp_msg;
}

void SocketRpcDriver::abortConnection()
{
	if(m_socket) {
		m_socket->abort();
	}
}

namespace {
int nextRpcId()
{
	static int n = 0;
	return ++n;
}
}
int SocketRpcDriver::callMethodASync(const std::string & method, const cp::RpcValue &params)
{
	int id = nextRpcId();
	cp::RpcRequest rq;
	rq.setId(id);
	rq.setMethod(method);
	rq.setParams(params);
	sendMessage(rq.value());
	return id;
}

void SocketRpcDriver::sendResponse(int request_id, const cp::RpcValue &result)
{
	cp::RpcResponse resp;
	resp.setId(request_id);
	resp.setResult(result);
	sendMessage(resp.value());
}

void SocketRpcDriver::sendError(int request_id, const cp::RpcResponse::Error &error)
{
	cp::RpcResponse resp;
	resp.setId(request_id);
	resp.setError(error);
	sendMessage(resp.value());
}

std::string SocketRpcDriver::peerAddress() const
{
	if(m_socket)
		return m_socket->peerAddress().toString().toStdString();
	return std::string();
}

int SocketRpcDriver::peerPort() const
{
	if(m_socket)
		return m_socket->peerPort();
	return -1;
}

}}}
