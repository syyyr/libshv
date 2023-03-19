#pragma once

#include <QSerialPort>
#include <QFile>

namespace shv::chainpack { class RpcMessage; }

class MockSerialPort : public QSerialPort
{
public:
	explicit MockSerialPort(const QString &name, QObject *parent = nullptr);

	bool open(OpenMode mode) override;
	void close() override;

	QByteArray writeData() const { return m_writeData; }
protected:
	qint64 writeData(const char *data, qint64 len) override;
	//enum class ForAppend {No, Yes};
	//void openWriteFile(QFile &file, ForAppend for_append);
private:
	QFile m_writeFile;
	QByteArray m_writeData;
};

