#include "socket.h"

#include <QHostAddress>
#include <QTcpSocket>
//#include <QWebSocket>

namespace shv {
namespace iotqt {
namespace rpc {

//======================================================
// Socket
//======================================================
Socket::Socket(QObject *parent)
	: QObject(parent)
{

}

//======================================================
// TcpSocket
//======================================================
TcpSocket::TcpSocket(QTcpSocket *socket, QObject *parent)
	: Super(parent)
	, m_socket(socket)
{
	m_socket->setParent(this);

	connect(m_socket, &QTcpSocket::connected, this, &Socket::connected);
	connect(m_socket, &QTcpSocket::disconnected, this, &Socket::disconnected);
	connect(m_socket, &QTcpSocket::readyRead, this, &Socket::readyRead);
	connect(m_socket, &QTcpSocket::bytesWritten, this, &Socket::bytesWritten);
	connect(m_socket, &QTcpSocket::stateChanged, this, &Socket::stateChanged);
	connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Socket::error);
}

void TcpSocket::connectToHost(const QString &host_name, quint16 port)
{
	m_socket->connectToHost(host_name, port);
}

void TcpSocket::close()
{
	m_socket->close();
}

void TcpSocket::abort()
{
	m_socket->abort();
}

QAbstractSocket::SocketState TcpSocket::state() const
{
	return m_socket->state();
}

QString TcpSocket::errorString() const
{
	return m_socket->errorString();
}

QHostAddress TcpSocket::peerAddress() const
{
	return m_socket->peerAddress();
}

quint16 TcpSocket::peerPort() const
{
	return m_socket->peerPort();
}

QByteArray TcpSocket::readAll()
{
	return m_socket->readAll();
}

qint64 TcpSocket::write(const char *data, qint64 max_size)
{
	return m_socket->write(data, max_size);
}

void TcpSocket::writeMessageEnd()
{
	m_socket->flush();
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
