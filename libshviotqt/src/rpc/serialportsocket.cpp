#include "serialportsocket.h"

#include <shv/chainpack/utils.h>

#include <QHostAddress>
#include <QSerialPort>
#include <QUrl>
#include <QTimer>

#include <optional>

namespace shv::iotqt::rpc {

//======================================================
// SerialPortSocket
//======================================================
SerialPortSocket::SerialPortSocket(QSerialPort *port, QObject *parent)
	: Super(parent)
	, m_port(port)
{
	m_readDataTimeout = new QTimer(this);
	m_readDataTimeout->setSingleShot(true);
	m_readDataTimeout->setInterval(5000);
	connect(m_readDataTimeout, &QTimer::timeout, this, [this]() {
		setReadMessageError(ReadMessageError::ErrorTimeout);
	});
	m_port->setParent(this);

	//connect(m_port, &QSerialPort::connected, this, &Socket::connected);
	//connect(m_port, &QSerialPort::disconnected, this, &Socket::disconnected);
	connect(m_port, &QSerialPort::readyRead, this, &SerialPortSocket::onSerialDataReadyRead);
	connect(m_port, &QSerialPort::bytesWritten, this, &Socket::bytesWritten);
	//connect(m_port, &QSerialPort::stateChanged, this, [this](QLocalSocket::LocalSocketState state) {
	//	emit stateChanged(LocalSocket_convertState(state));
	//});
	connect(m_port, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError port_error) {
		switch (port_error) {
		case QSerialPort::NoError:
			break;
		case QSerialPort::DeviceNotFoundError:
			emit error(QAbstractSocket::HostNotFoundError);
			break;
		case QSerialPort::PermissionError:
			emit error(QAbstractSocket::SocketAccessError);
			break;
		case QSerialPort::OpenError:
			emit error(QAbstractSocket::ConnectionRefusedError);
			break;
#if QT_VERSION_MAJOR < 6
		case QSerialPort::ParityError:
		case QSerialPort::FramingError:
		case QSerialPort::BreakConditionError:
#endif
		case QSerialPort::WriteError:
		case QSerialPort::ReadError:
			emit error(QAbstractSocket::OperationError);
			break;
		case QSerialPort::ResourceError:
			emit error(QAbstractSocket::SocketResourceError);
			break;
		case QSerialPort::UnsupportedOperationError:
			emit error(QAbstractSocket::OperationError);
			break;
		case QSerialPort::UnknownError:
			emit error(QAbstractSocket::UnknownSocketError);
			break;
		case QSerialPort::TimeoutError:
			emit error(QAbstractSocket::SocketTimeoutError);
			break;
		case QSerialPort::NotOpenError:
			emit error(QAbstractSocket::SocketAccessError);
			break;
		}
	});
}

void SerialPortSocket::connectToHost(const QUrl &url)
{
	abort();
	setState(QAbstractSocket::ConnectingState);
	m_port->setPortName(url.path());
	shvInfo() << "opening serial port:" << m_port->portName();
	if(m_port->open(QIODevice::ReadWrite)) {
		shvInfo() << "Ok";
		// clear read buffer
		//m_port->readAll();
		setState(QAbstractSocket::ConnectedState);
	}
	else {
		shvWarning() << "Error open serial port:" << m_port->portName() << m_port->errorString();
		setState(QAbstractSocket::UnconnectedState);
	}
}

void SerialPortSocket::close()
{
	if(state() == QAbstractSocket::UnconnectedState)
		return;
	setState(QAbstractSocket::ClosingState);
	m_port->close();
	setState(QAbstractSocket::UnconnectedState);
}

void SerialPortSocket::abort()
{
	close();
}

QString SerialPortSocket::errorString() const
{
	switch(m_readMessageError) {
	case ReadMessageError::Ok:
		return m_port->errorString();
	case ReadMessageError::ErrorEscape: return QStringLiteral("Escaping error");
	case ReadMessageError::ErrorCrc: return QStringLiteral("CRC error");
	case ReadMessageError::ErrorTimeout: return QStringLiteral("Timeout error");
		break;
	}
	return {};
}

QHostAddress SerialPortSocket::peerAddress() const
{
	return QHostAddress(m_port->portName());
}

quint16 SerialPortSocket::peerPort() const
{
	return 0;
}

void SerialPortSocket::onSerialDataReadyRead()
{
	auto escaped_data = m_port->readAll();
#if QT_VERSION_MAJOR < 6
	int ix = 0;
#else
	qsizetype ix = 0;
#endif
	auto try_get_byte = [&escaped_data, &ix, this]() -> std::optional<uint8_t> {
		std::optional<uint8_t> ret;
		auto b = static_cast<uint8_t>(escaped_data[ix++]);
		if(m_prevByteRead == ESC) {
			if(b == ESTX) {
				ret = STX;
			}
			else if(b == EETX) {
				ret = ETX;
			}
			else if(b == ESC) {
				ret = ESC;
			}
			else {
				setReadMessageError(ReadMessageError::ErrorEscape);
			}
		}
		else if(b == ESC) {
		}
		else {
			ret = b;
		}
		m_prevByteRead = b;
		return ret;
	};
	while(ix < escaped_data.size()) {
		switch(m_readMessageState) {
		case ReadMessageState::WaitingForStx: {
			for (; ix < escaped_data.size(); ++ix) {
				if(static_cast<uint8_t>(escaped_data[ix]) == STX) {
					ix++;
					setReadMessageState(ReadMessageState::WaitingForEtx);
					break;
				}
			}
			break;
		}
		case ReadMessageState::WaitingForEtx: {
			while (ix < escaped_data.size()) {
				auto b = try_get_byte();
				if(b.has_value()) {
					if(b == ETX) {
						setReadMessageState(ReadMessageState::WaitingForCrc);
						break;
					}
					else {
						m_readMessageData.append(b.value());
					}
				}
			}
			break;
		}
		case ReadMessageState::WaitingForCrc: {
			while (ix < escaped_data.size()) {
				auto b = try_get_byte();
				if(b.has_value()) {
					m_readMessageCrc.append(b.value());
					if(m_readMessageCrc.size() == sizeof(shv::chainpack::crc32_t )) {
						shv::chainpack::crc32_t msg_crc = 0;
						for(uint8_t bb : m_readMessageCrc) {
							msg_crc <<= 8;
							msg_crc += bb;
						}
						shv::chainpack::Crc32Posix crc32;
						crc32.add(STX);
						crc32.add(m_readMessageData.constData(), m_readMessageData.size());
						crc32.add(ETX);
						//shvWarning() << "read msg:" << m_readMessageData.toHex().toStdString();
						//shvWarning() << "crc data:" << m_readMessageCrc.toHex().toStdString();
						//shvWarning() << "crc:" << shv::chainpack::utils::intToHex(crc32.remainder());
						if(crc32.remainder() == msg_crc) {
							setReadMessageState(ReadMessageState::WaitingForStx);
							emit readyRead();
						}
						else {
							setReadMessageError(ReadMessageError::ErrorCrc);
						}
					}
				}
			}
			break;
		}
		}
	}
}

void SerialPortSocket::setReadMessageState(ReadMessageState st)
{
	if(st == ReadMessageState::WaitingForEtx) {
		m_readDataTimeout->start();
		m_readMessageData.clear();
		m_prevByteRead = 0;
	}
	else if(st == ReadMessageState::WaitingForCrc) {
		m_readMessageCrc.clear();
		m_prevByteRead = 0;
	}
	m_readMessageState = st;
}

void SerialPortSocket::setReadMessageError(ReadMessageError err)
{
	m_readMessageError = err;
	setReadMessageState(ReadMessageState::WaitingForStx);
	emit error(QAbstractSocket::NetworkError);
}

qint64 SerialPortSocket::writeBytesEscaped(const char *data, qint64 max_size)
{
	char arr[] = {0};
	auto set_byte = [&arr](uint8_t b) {
		arr[0] = static_cast<char>(b);
	};
	qint64 written_cnt = 0;
	for(qint64 i = 0; i < max_size; ++i) {
		auto b = static_cast<uint8_t>(data[i]);
		if(b == STX) {
			if(m_prevByteWrite == ESC) {
				set_byte(ESTX);
			}
			else {
				set_byte(ESC);
				--i;
			}
		}
		else if(b == ETX) {
			if(m_prevByteWrite == ESC) {
				set_byte(EETX);
			}
			else {
				set_byte(ESC);
				--i;
			}
		}
		else if(b == ESC) {
			if(m_prevByteWrite == ESC) {
				set_byte(ESC);
			}
			else {
				set_byte(ESC);
				--i;
			}
		}
		else {
			set_byte(b);
		}
		//shvError() << "write byte:" << shv::chainpack::utils::byteToHex(arr[0]);
		m_writeMessageCrc.add(arr[0]);
		auto n = m_port->write(arr, 1);
		if(n < 0) {
			return -1;
		}
		if(n == 0) {
			return written_cnt;
		}
		written_cnt += n;
		m_prevByteWrite = arr[0];
	}
	return written_cnt;
}

QByteArray SerialPortSocket::readAll()
{
	if(m_readMessageState == ReadMessageState::WaitingForStx) {
		auto ret = m_readMessageData;
		m_readMessageData = {};
		return ret;
	}
	return {};
}

qint64 SerialPortSocket::write(const char *data, qint64 max_size)
{
	return writeBytesEscaped(data, max_size);
}

void SerialPortSocket::writeMessageBegin()
{
	m_writeMessageCrc = {};
	m_writeMessageCrc.add(STX);
	const char stx[] = {static_cast<char>(STX)};
	m_port->write(stx, 1);
}

void SerialPortSocket::writeMessageEnd()
{
	const char etx[] = {static_cast<char>(ETX)};
	m_port->write(etx, 1);
	m_writeMessageCrc.add(ETX);
	auto crc = m_writeMessageCrc.remainder();
	static constexpr size_t N = sizeof(shv::chainpack::crc32_t);
	QByteArray crc_ba(N, 0);
	for(size_t i = 0; i < N; i++) {
#if QT_VERSION_MAJOR < 6
		crc_ba[static_cast<uint>(N - i - 1)] = static_cast<char>(crc % 256);
#else
		crc_ba[N - i - 1] = static_cast<char>(crc % 256);
#endif
		crc /= 256;
	}
	writeBytesEscaped(crc_ba.constData(), crc_ba.size());
	m_port->flush();
}

void SerialPortSocket::setState(QAbstractSocket::SocketState state)
{
	if(state == m_state)
		return;
	m_state = state;
	emit stateChanged(m_state);
	if(m_state == QAbstractSocket::SocketState::ConnectedState)
		emit connected();
	else if(m_state == QAbstractSocket::SocketState::UnconnectedState)
		emit disconnected();
}

void SerialPortSocket::onParseDataException(const chainpack::ParseException &)
{
	// nothing to do
}

}
