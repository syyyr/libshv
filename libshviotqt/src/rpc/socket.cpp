#include "socket.h"

#include <shv/coreqt/log.h>

#include <QHostAddress>
#include <QSslConfiguration>
#include <QSslError>
#include <QTcpSocket>
#include <QTimer>

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
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
	connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &Socket::error);
#else
	connect(m_socket, &QAbstractSocket::errorOccurred, this, &Socket::error);
#endif
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
	/// direct flush in QSslSocket call can cause readyRead() emit
	/// QSslSocketBackendPrivate::transmit() is badly implemented
	/// It does not hurt to comment this, since flush() called automatically from event loop anyway
	//m_socket->flush(); // bacha na to, je to zlo
	//QTimer::singleShot(0, m_socket, &QTcpSocket::flush);
}

//======================================================
// SslSocket
//======================================================
SslSocket::SslSocket(QSslSocket *socket, QSslSocket::PeerVerifyMode peer_verify_mode, QObject *parent)
	: Super(socket, parent), m_peerVerifyMode(peer_verify_mode)
{
	/*
	{
		QSslConfiguration ssl_configuration = socket->sslConfiguration();
		shvDebug() << "SSL socket config:";
		shvDebug() << "protocol:" << (int)ssl_configuration.protocol();
		//ssl_configuration.setProtocol(QSsl::TlsV1SslV3);
		//socket->setSslConfiguration(ssl_configuration);
	}
	*/
	disconnect(m_socket, &QTcpSocket::connected, this, &Socket::connected);
	connect(socket, &QSslSocket::encrypted, this, &Socket::connected);
}

void SslSocket::connectToHost(const QString &host_name, quint16 port)
{
	QSslSocket *ssl_socket = qobject_cast<QSslSocket *>(m_socket);
	ssl_socket->setPeerVerifyMode(m_peerVerifyMode);
	shvDebug() << "connectToHostEncrypted" << "host:" << host_name << "port:" << port;
	ssl_socket->connectToHostEncrypted(host_name, port);
}

} // namespace rpc
} // namespace iotqt
} // namespace shv
