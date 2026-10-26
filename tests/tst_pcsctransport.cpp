// Only behaviour that does not need a reader/card or a running pcscd is covered here.
// Card-level behaviour is covered with MockTransport in tst_cardsession.cpp.
#include "testregistry.h"
#include "card/pcsctransport.h"

#include <QTest>
#include <QSignalSpy>

class TstPcscTransport : public QObject
{
    Q_OBJECT

private slots:
    void notConnectedByDefault();
    void sendWithoutCardLogsErrorAndReturnsEmpty();
    void resetWithoutCardReturnsFalse();
    void disconnectWithoutCardReportsFailure();
    void connectToUnknownReaderFails();
    void getReadersDoesNotCrashWithoutService();
};

void TstPcscTransport::notConnectedByDefault()
{
    PcscTransport pcsc;
    QVERIFY(!pcsc.isConnected());
}

void TstPcscTransport::sendWithoutCardLogsErrorAndReturnsEmpty()
{
    PcscTransport pcsc;
    QSignalSpy log(&pcsc, &PcscTransport::log);

    QVERIFY(pcsc.transmit(QByteArray::fromHex("00A40400")).isEmpty());

    QCOMPARE(log.count(), 1);
    QCOMPARE(log.first().at(0).value<LogType>(), LogType::Error);
}

void TstPcscTransport::resetWithoutCardReturnsFalse()
{
    PcscTransport pcsc;
    QVERIFY(!pcsc.reset());
}

void TstPcscTransport::disconnectWithoutCardReportsFailure()
{
    PcscTransport pcsc;

    QVERIFY(!pcsc.disconnectCard());
    QVERIFY(!pcsc.isConnected());
}

void TstPcscTransport::connectToUnknownReaderFails()
{
    PcscTransport pcsc;
    QVERIFY(!pcsc.connectCard("no such reader"));
    QVERIFY(!pcsc.isConnected());
}

void TstPcscTransport::getReadersDoesNotCrashWithoutService()
{
    PcscTransport pcsc;
    const QStringList readers = pcsc.readers();   // empty or real readers, both fine
    Q_UNUSED(readers)
    QVERIFY(true);
}

static TestRegistrar<TstPcscTransport> registrar;
#include "tst_pcsctransport.moc"
