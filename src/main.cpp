#include "mainwindow.h"

#include <QApplication>
#include <QSharedMemory>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>


int main(int argc, char *argv[])
{
    qDebug() << "Starting SmartCardManager";
    QApplication a(argc, argv);

    a.setApplicationName("SmartCardManager");
    a.setApplicationVersion("0.1");
    a.setOrganizationName("couleurq@gmail.com");
    a.setOrganizationDomain("yourdomain.com");

    QLocalSocket socket;
    const QString UNIQUE_KEY = "SmartCardManagerSingleInstance";
        socket.connectToServer(UNIQUE_KEY);

    if (socket.waitForConnected(500)) {
        // Another instance is already running
        qDebug() << "Another instance is running";
        return 0;
    }

    // No other instance is running, so this is the first instance
    QLocalServer server;
    if (!server.listen(UNIQUE_KEY)) {
        qWarning() << "Cannot start in single instance mode";
    }

    // create the main window
    MainWindow w;
    w.resize(800, 600);
    w.show();

    // listen code to raise the window again instead of second instance
    QObject::connect(&server, &QLocalServer::newConnection, &w, [&]() {
        QLocalSocket *clientSocket = server.nextPendingConnection();
        if (clientSocket) {
            clientSocket->waitForReadyRead();
            qDebug() << "Socket connection received. Raising window.";
            w.activateWindow();
            w.raise();
            w.show();
        }
    });


    return a.exec();
}
