#include "testregistry.h"
#include "mocktransport.h"
#include "card/cardsession.h"
#include "commands/commandsendbytes.h"
#include "commands/commandapdu.h"
#include "commands/commandterminal.h"

#include <QTest>
#include <QSignalSpy>

class TstCardSession : public QObject
{
    Q_OBJECT

private slots:
    void readersComeFromTransport();
    void connectUsesSelectedReader();
    void connectWithoutReaderFails();
    void changingReaderDropsConnection();
    void stateChangedIsEmitted();
    void transportLogIsForwarded();
    void sendAsyncDeliversResponseOnGuiThread();
    void sendAsyncKeepsOrder();
    void sendAsyncWithoutCardReportsEmptyResponse();
    void sendBytesExecuteChecksExpectedResponse();
    void apduIsBuilt();
    void apduExecuteSendsBuiltApdu();
    void terminalCommandsDriveSession();
};

void TstCardSession::readersComeFromTransport()
{
    MockTransport transport;
    CardSession session(&transport);
    QCOMPARE(session.readers(), transport.readerList);
}

void TstCardSession::connectUsesSelectedReader()
{
    MockTransport transport;
    CardSession session(&transport);
    session.setReader("Mock Reader 1");

    QVERIFY(session.connectCard());
    QVERIFY(session.isConnected());
    QCOMPARE(transport.connectedReader, QString("Mock Reader 1"));

    QVERIFY(session.reset());
    QCOMPARE(transport.resetCount, 1);

    QVERIFY(session.disconnectCard());
    QVERIFY(!session.isConnected());
}

void TstCardSession::connectWithoutReaderFails()
{
    MockTransport transport;
    CardSession session(&transport);
    QVERIFY(!session.connectCard());
    QVERIFY(!session.isConnected());
}

void TstCardSession::changingReaderDropsConnection()
{
    MockTransport transport;
    CardSession session(&transport);
    session.setReader("Mock Reader 0");
    session.connectCard();

    session.setReader("Mock Reader 1");

    QVERIFY(!session.isConnected());
    QCOMPARE(session.reader(), QString("Mock Reader 1"));
}

void TstCardSession::stateChangedIsEmitted()
{
    MockTransport transport;
    CardSession session(&transport);
    QSignalSpy spy(&session, &CardSession::stateChanged);

    session.setReader("Mock Reader 0");
    session.connectCard();
    session.reset();
    session.disconnectCard();

    QCOMPARE(spy.count(), 4);
}

void TstCardSession::transportLogIsForwarded()
{
    MockTransport transport;
    CardSession session(&transport);
    QSignalSpy spy(&session, &CardSession::log);

    session.transmit(QByteArray::fromHex("00"));   // not connected -> transport logs an error

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().at(0).value<LogType>(), LogType::Error);
}

void TstCardSession::sendAsyncDeliversResponseOnGuiThread()
{
    MockTransport transport;
    transport.answers.insert(QByteArray::fromHex("00A40400"), QByteArray::fromHex("6A82"));
    transport.transmitDelayMs = 50;
    CardSession session(&transport);
    session.setReader("Mock Reader 0");
    session.connectCard();

    QThread *receiverThread = nullptr;
    QByteArray received;
    connect(&session, &CardSession::responseReceived, this, [&](const QByteArray &, const QByteArray &response) {
        receiverThread = QThread::currentThread();
        received = response;
    });

    QElapsedTimer timer;
    timer.start();
    session.sendAsync(QByteArray::fromHex("00A40400"));
    QVERIFY2(timer.elapsed() < 40, "sendAsync must not block on the transmission");

    QTRY_VERIFY(!received.isEmpty());
    QCOMPARE(received, QByteArray::fromHex("6A82"));
    QCOMPARE(receiverThread, QThread::currentThread());
}

void TstCardSession::sendAsyncKeepsOrder()
{
    MockTransport transport;
    CardSession session(&transport);
    session.setReader("Mock Reader 0");
    session.connectCard();
    QSignalSpy spy(&session, &CardSession::responseReceived);

    for (int i = 0; i < 5; ++i)
        session.sendAsync(QByteArray(1, static_cast<char>(i)));

    QTRY_COMPARE(spy.count(), 5);
    for (int i = 0; i < 5; ++i)
        QCOMPARE(spy.at(i).at(0).toByteArray(), QByteArray(1, static_cast<char>(i)));
}

void TstCardSession::sendAsyncWithoutCardReportsEmptyResponse()
{
    MockTransport transport;
    CardSession session(&transport);
    QSignalSpy spy(&session, &CardSession::responseReceived);

    session.sendAsync(QByteArray::fromHex("00"));

    QTRY_COMPARE(spy.count(), 1);
    QVERIFY(spy.first().at(1).toByteArray().isEmpty());
}

void TstCardSession::sendBytesExecuteChecksExpectedResponse()
{
    MockTransport transport;
    transport.answers.insert(QByteArray::fromHex("0102"), QByteArray::fromHex("9000"));
    CardSession session(&transport);
    session.setReader("Mock Reader 0");
    session.connectCard();

    CommandSendBytes cmd;
    cmd.commandData = QByteArray::fromHex("0102");

    QByteArray response;
    QVERIFY(cmd.execute(session, &response));
    QCOMPARE(response, QByteArray::fromHex("9000"));

    cmd.expectedResponse = QByteArray::fromHex("9000");
    QVERIFY(cmd.execute(session));
    cmd.expectedResponse = QByteArray::fromHex("6A82");
    QVERIFY(!cmd.execute(session));
}

void TstCardSession::apduIsBuilt()
{
    CommandApdu apdu;
    apdu.cla = QByteArray::fromHex("00");
    apdu.ins = QByteArray::fromHex("A4");
    apdu.p1 = QByteArray::fromHex("04");
    apdu.p2 = QByteArray::fromHex("00");
    QCOMPARE(apdu.buildApdu(), QByteArray::fromHex("00A40400"));

    apdu.commandData = QByteArray::fromHex("A000000151");
    apdu.le = QByteArray::fromHex("00");
    QCOMPARE(apdu.buildApdu(), QByteArray::fromHex("00A4040005A00000015100"));

    apdu.commandData = QByteArray(300, 'x');
    apdu.le.clear();
    const QByteArray extended = apdu.buildApdu();
    QCOMPARE(extended.left(7), QByteArray::fromHex("00A40400") + QByteArray::fromHex("00012C"));
    QCOMPARE(extended.size(), 7 + 300);
}

void TstCardSession::apduExecuteSendsBuiltApdu()
{
    MockTransport transport;
    CardSession session(&transport);
    session.setReader("Mock Reader 0");
    session.connectCard();

    CommandApdu apdu;
    apdu.cla = QByteArray::fromHex("80");
    apdu.ins = QByteArray::fromHex("CA");
    apdu.p1 = QByteArray::fromHex("9F");
    apdu.p2 = QByteArray::fromHex("7F");
    apdu.le = QByteArray::fromHex("00");

    QVERIFY(apdu.execute(session));
    QCOMPARE(transport.sent.last(), QByteArray::fromHex("80CA9F7F00"));
}

void TstCardSession::terminalCommandsDriveSession()
{
    MockTransport transport;
    CardSession session(&transport);
    session.setReader("Mock Reader 0");

    CommandTerminal cmd;
    cmd.action = CommandTerminal::ActionType::Connect;
    QVERIFY(cmd.execute(session));
    QVERIFY(session.isConnected());

    cmd.action = CommandTerminal::ActionType::WarmReset;
    QVERIFY(cmd.execute(session));
    QCOMPARE(transport.resetCount, 1);

    cmd.action = CommandTerminal::ActionType::Disconnect;
    QVERIFY(cmd.execute(session));
    QVERIFY(!session.isConnected());

    cmd.action = CommandTerminal::ActionType::None;
    QVERIFY(!cmd.execute(session));
}

static TestRegistrar<TstCardSession> registrar;
#include "tst_cardsession.moc"
