#include "websocket.h"

#include <shv/coreqt/log.h>

#include <QWebSocket>

namespace shv {
namespace iotqt {
namespace rpc {

WebSocket::WebSocket(QWebSocket *socket, QObject *parent)
	: Super(parent)
	, m_socket(socket)
{
	m_socket->setParent(this);

	connect(m_socket, &QWebSocket::connected, this, &Socket::connected);
	connect(m_socket, &QWebSocket::disconnected, this, &Socket::disconnected);
	connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocket::onTextMessageReceived);
	connect(m_socket, &QWebSocket::binaryMessageReceived, this, &WebSocket::onBinaryMessageReceived);
	connect(m_socket, &QWebSocket::bytesWritten, this, &Socket::bytesWritten);
	connect(m_socket, &QWebSocket::stateChanged, this, &Socket::stateChanged);
	connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &Socket::error);
#ifndef QT_NO_SSL
	connect(m_socket, &QWebSocket::sslErrors, this, &Socket::sslErrors);
#endif
}

void WebSocket::connectToHost(const QString &host_name, quint16 port, const QString &scheme)
{
	QUrl url;
	url.setScheme(scheme);
	url.setHost(host_name);
	url.setPort(port);
	shvInfo() << "connecting to:" << url.toString();
	m_socket->open(url);
}

void WebSocket::close()
{
	m_socket->close();
}

void WebSocket::abort()
{
	m_socket->abort();
}

QAbstractSocket::SocketState WebSocket::state() const
{
	return m_socket->state();
}

QString WebSocket::errorString() const
{
	return m_socket->errorString();
}

QHostAddress WebSocket::peerAddress() const
{
	return m_socket->peerAddress();
}

quint16 WebSocket::peerPort() const
{
	return m_socket->peerPort();
}

QByteArray WebSocket::readAll()
{
	QByteArray ret = m_readBuffer;
	m_readBuffer.clear();
	return ret;
}

qint64 WebSocket::write(const char *data, qint64 data_size)
{
	QByteArray ba(data, static_cast<int>(data_size));
	m_writeBuffer.append(ba);
	return data_size;
}

void WebSocket::writeMessageBegin()
{
	shvDebug() << __FUNCTION__;
	m_writeBuffer.clear();
}

void WebSocket::writeMessageEnd()
{
	shvDebug() << __FUNCTION__ << "message len:" << m_writeBuffer.size() << "\n" << m_writeBuffer;
	qint64 n = m_socket->sendBinaryMessage(m_writeBuffer);
	if(n < m_writeBuffer.size())
		shvError() << "Send message error, only" << n << "bytes written.";
	m_socket->flush();
}

void WebSocket::ignoreSslErrors()
{
#ifndef QT_NO_SSL
	m_socket->ignoreSslErrors();
#endif
}

void WebSocket::onTextMessageReceived(const QString &message)
{
	shvDebug() << "text message received:" << message;
	m_readBuffer.append(message.toUtf8());
	emit readyRead();
}

void WebSocket::onBinaryMessageReceived(const QByteArray &message)
{
	shvDebug() << "binary message received:" << message;
	m_readBuffer.append(message);
	emit readyRead();
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
