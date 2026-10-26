// Only behaviour that does not need a reader/card or a running pcscd is covered here.
// Card-level behaviour needs the transport abstraction proposed in docs/CODE_REVIEW.md.
#include "testregistry.h"
#include "pcscwrapper.h"

#include <QTest>
#include <QSignalSpy>

class TstPcscWrapper : public QObject
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

void TstPcscWrapper::notConnectedByDefault()
{
    PcscWrapper pcsc;
    QVERIFY(!pcsc.isConnected("any reader"));
}

void TstPcscWrapper::sendWithoutCardLogsErrorAndReturnsEmpty()
{
    PcscWrapper pcsc;
    QSignalSpy log(&pcsc, &PcscWrapper::log);

    QVERIFY(pcsc.send("reader", QByteArray::fromHex("00A40400")).isEmpty());

    QCOMPARE(log.count(), 1);
    QCOMPARE(log.first().at(0).value<LogType>(), LogType::Error);
}

void TstPcscWrapper::resetWithoutCardReturnsFalse()
{
    PcscWrapper pcsc;
    QVERIFY(!pcsc.reset("reader"));
}

void TstPcscWrapper::disconnectWithoutCardReportsFailure()
{
    PcscWrapper pcsc;

    QVERIFY(!pcsc.disconnect("reader"));
    QVERIFY(!pcsc.isConnected("reader"));
}

void TstPcscWrapper::connectToUnknownReaderFails()
{
    PcscWrapper pcsc;
    QVERIFY(!pcsc.connect("no such reader"));
    QVERIFY(!pcsc.isConnected("no such reader"));
}

void TstPcscWrapper::getReadersDoesNotCrashWithoutService()
{
    PcscWrapper pcsc;
    const QStringList readers = pcsc.getReaders();   // empty or real readers, both fine
    Q_UNUSED(readers)
    QVERIFY(true);
}

static TestRegistrar<TstPcscWrapper> registrar;
#include "tst_pcscwrapper.moc"
