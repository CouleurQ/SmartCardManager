#include "card/cardsession.h"
#include "card/pcsctransport.h"
#include "ui/mainwindow.h"

#include <QApplication>
#include <QDebug>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSocketNotifier>
#include <QDir>

#include <csignal>
#include <unistd.h>
#include <sys/socket.h>


namespace {
int signalFd[2] = {-1, -1};

// Async-signal-safe: only write one byte to the socketpair, the Qt event loop does the rest.
void signalHandler(int signal)
{
    const char byte = static_cast<char>(signal);
    ssize_t ignored = ::write(signalFd[0], &byte, 1);
    (void) ignored;
}

void installSignalHandlers(QObject *parent)
{
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, signalFd) != 0)
        return;

    auto *notifier = new QSocketNotifier(signalFd[1], QSocketNotifier::Read, parent);
    QObject::connect(notifier, &QSocketNotifier::activated, parent, [notifier]() {
        notifier->setEnabled(false);
        char byte;
        ssize_t ignored = ::read(signalFd[1], &byte, 1);
        (void) ignored;
        qDebug() << "Received signal" << static_cast<int>(byte) << "- shutting down";
        QCoreApplication::quit();
    });

    struct sigaction action {};
    action.sa_handler = signalHandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &action, nullptr);
    sigaction(SIGTERM, &action, nullptr);
}
}


int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    application.setApplicationName("SmartCardManager");
    application.setApplicationVersion("0.1");
    application.setOrganizationName("CouleurQ");
    application.setOrganizationDomain("github.com");

    installSignalHandlers(&application);

    const QString socketPath = QDir::tempPath() + QDir::separator() + QCoreApplication::applicationName();
    QLocalSocket socket;
    socket.connectToServer(socketPath);

    if (socket.waitForConnected(500)) {
        // Another instance is already running - the connection itself tells it to raise its window
        qDebug() << "Switching to already existing instance";
        return 0;
    }

    // No other instance is running. A leftover socket file of a crashed instance would block listen().
    QLocalServer::removeServer(socketPath);
    QLocalServer server;
    if (!server.listen(socketPath))
        qWarning() << "Cannot start in single instance mode";

    PcscTransport transport;
    CardSession session(&transport);

    MainWindow w(&session);
    w.resize(800, 600);
    w.show();

    QObject::connect(&server, &QLocalServer::newConnection, &w, [&]() {
        while (QLocalSocket *clientSocket = server.nextPendingConnection()) {
            clientSocket->deleteLater();
            qDebug() << "Socket connection received. Raising window.";
            w.activateWindow();
            w.raise();
            w.show();
        }
    });

    return application.exec();
}
