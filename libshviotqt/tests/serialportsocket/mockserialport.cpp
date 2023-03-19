#include "mockserialport.h"

#include <shv/chainpack/utils.h>

#include <QFile>

#include <necrolog.h>

MockSerialPort::MockSerialPort(const QString &name,  QObject *parent)
	: QSerialPort{parent}
{
	Q_ASSERT(!name.isEmpty());
	setObjectName(name);
}

bool MockSerialPort::open(OpenMode mode)
{
	Q_UNUSED(mode);
	close();
	{
		auto fname = "/tmp/serialportsocket-test-write" + objectName() + ".bin";
		m_writeFile.setFileName(fname);
		if(!m_writeFile.open(QFile::WriteOnly)) {
			throw "Canot open file: " + fname + " for writing";
		}
	}
	QIODevice::open(mode);
	return true;
}

void MockSerialPort::close()
{
	m_writeFile.close();
	m_writeData.clear();
	QIODevice::close();
}

qint64 MockSerialPort::writeData(const char *data, qint64 len)
{
	m_writeData.append(data, len);
	return m_writeFile.write(data, len);
}

