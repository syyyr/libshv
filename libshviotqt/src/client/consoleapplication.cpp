#include "consoleapplication.h"
#include "../rpc/deviceconnection.h"
#include "appclioptions.h"

#include <shv/coreqt/log.h>
#include <shv/coreqt/utils/versioninfo.h>

#include <QSocketNotifier>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

//#define logOpcuaReceive qfCInfo("OpcuaReceive")

namespace shv {
namespace iotqt {
namespace client {

#ifdef Q_OS_UNIX
int ConsoleApplication::m_sigTermFd[2];
#endif

//static constexpr int SQL_RECONNECT_INTERVAL = 3000;

ConsoleApplication::ConsoleApplication(int &argc, char **argv, AppCliOptions *cli_opts)
	: Super(argc, argv)
	, m_cliOptions(cli_opts)
{
	shvInfo() << "creating SHV client application object ver." << versionString();
#ifdef Q_OS_UNIX
	//syslog (LOG_INFO, "Server started");
	installUnixSignalHandlers();
#endif
}

ConsoleApplication::~ConsoleApplication()
{
	shvInfo() << "Destroying SHV CLIENT application object";
	//QF_SAFE_DELETE(m_tcpServer);
	//QF_SAFE_DELETE(m_sqlConnector);
}

coreqt::utils::VersionInfo ConsoleApplication::versionInfo() const
{
	return shv::coreqt::utils::VersionInfo(versionString());
}

QString ConsoleApplication::versionString() const
{
	return QCoreApplication::applicationVersion();
}

rpc::ClientConnection *ConsoleApplication::clientConnection()
{
	if(!m_clientConnection)
		SHV_EXCEPTION("Client connection is NULL");
	return m_clientConnection;
}

void ConsoleApplication::setClientConnection(rpc::ClientConnection *cc)
{
	m_clientConnection = cc;
	if(m_clientConnection)
		m_checkConnectedTimer->start();
	else
		m_checkConnectedTimer->stop();
}

#ifdef Q_OS_UNIX
void ConsoleApplication::installUnixSignalHandlers()
{
	shvInfo() << "installing Unix signals handlers";
	{
		struct sigaction term;

		term.sa_handler = ConsoleApplication::sigTermHandler;
		sigemptyset(&term.sa_mask);
		term.sa_flags |= SA_RESTART;

		if(sigaction(SIGTERM, &term, 0) > 0)
			qFatal("Couldn't register SIG_TERM handler");
	}
	if(::socketpair(AF_UNIX, SOCK_STREAM, 0, m_sigTermFd))
		qFatal("Couldn't create SIG_TERM socketpair");
	m_snTerm = new QSocketNotifier(m_sigTermFd[1], QSocketNotifier::Read, this);
	connect(m_snTerm, &QSocketNotifier::activated, this, &ConsoleApplication::handleSigTerm);
	shvInfo() << "SIG_TERM handler installed OK";
}

void ConsoleApplication::sigTermHandler(int)
{
	shvInfo() << "SIG TERM";
	char a = 1;
	::write(m_sigTermFd[0], &a, sizeof(a));
}

void ConsoleApplication::handleSigTerm()
{
	m_snTerm->setEnabled(false);
	char tmp;
	::read(m_sigTermFd[1], &tmp, sizeof(tmp));

	shvInfo() << "SIG TERM catched, stopping agent.";
	QMetaObject::invokeMethod(this, "quit", Qt::QueuedConnection);

	m_snTerm->setEnabled(true);
}
#endif

} // namespace client
} // namespace iot
} // namespace shv
