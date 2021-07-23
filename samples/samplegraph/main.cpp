#include "mainwindow.h"

#include <necrolog.h>

#include <QApplication>

int main(int argc, char *argv[])
{
	QCoreApplication::setOrganizationName("Elektroline");
	QCoreApplication::setOrganizationDomain("elektroline.cz");
	QCoreApplication::setApplicationName("sampleshvbroker");
	QCoreApplication::setApplicationVersion("0.0.1");

	NecroLog::setCLIOptions(argc, argv);

	QApplication a(argc, argv);
	MainWindow w;
	w.show();
	return a.exec();
}
