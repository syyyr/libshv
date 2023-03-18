#pragma once

#include "../shviotqtglobal.h"
#include "socket.h"

#include <shv/chainpack/crc32.h>
#include <shv/coreqt/log.h>

#include <QObject>
#include <QAbstractSocket>
#include <QSslSocket>
#include <QSslError>

class QSerialPort;
class QTimer;

namespace shv::iotqt::rpc {

class SHVIOTQT_DECL_EXPORT SerialPortSocket : public Socket
{
	Q_OBJECT

	using Super = Socket;
public:
	SerialPortSocket(QSerialPort *port, QObject *parent = nullptr);

	void connectToHost(const QUrl &url) override;
	void close() override;
	void abort() override;
	QAbstractSocket::SocketState state() const override { return m_state; }
	QString errorString() const override;
	QHostAddress peerAddress() const override;
	quint16 peerPort() const override;
	QByteArray readAll() override;
	qint64 write(const char *data, qint64 max_size) override;
	//bool flush() override;
	void writeMessageBegin() override;
	void writeMessageEnd() override;
	void ignoreSslErrors() override {}
private:
	enum EscCodes {
		STX = 0xA2,
		ETX = 0xA3,
		ESTX = 0xA4,
		EETX = 0xA5,
		ESC = 0xAA,
	};
	enum class ReadMessageState {WaitingForStx, WaitingForEtx, WaitingForCrc};
	enum class ReadMessageError {Ok = 0, ErrorEscape, ErrorCrc, ErrorTimeout};

	void setState(QAbstractSocket::SocketState state);
	void onSerialDataReadyRead();
	void onParseDataException(const shv::chainpack::ParseException &e) override;
	void setReadMessageState(ReadMessageState st);
	void setReadMessageError(ReadMessageError err);
	qint64 writeBytesEscaped(const char *data, qint64 max_size);
private:
	QSerialPort *m_port = nullptr;
	QAbstractSocket::SocketState m_state = QAbstractSocket::UnconnectedState;
	ReadMessageState m_readMessageState = ReadMessageState::WaitingForStx;
	ReadMessageError m_readMessageError = ReadMessageError::Ok;
	QTimer *m_readDataTimeout = nullptr;
	uint8_t m_prevByteRead = 0;
	QByteArray m_readMessageData;
	QByteArray m_readMessageCrc;

	uint8_t m_prevByteWrite = 0;
	shv::chainpack::Crc32Posix m_writeMessageCrc;
};
}
