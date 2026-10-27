#include "testregistry.h"
#include "mocktransport.h"
#include "privateaccess.h"
#include "ui_mainwindow.h"
#include "ui/atrview.h"

#include <QTest>
#include <QSignalSpy>

class TstMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void startsDisconnected();
    void toolbarHasReaderCombo();
    void logDockToggleFollowsVisibility();
    void logMessagesAppearInLogView();
    void sendCommandTabSignalsAreWired();
    void runScriptTabSignalsAreWired();
    void exitActionClosesWindow();
    void readersAreListedInTheBackground();
    void connectDisconnectResetRunInTheBackground();
    void cardRemovalSwitchesToDisconnectedState();
    void atrTabIsShownWhileConnected();
};

void TstMainWindow::startsDisconnected()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    w.show();
    QVERIFY(w.ui->actionConnect->isEnabled() == !session.isConnected());
    QVERIFY(!w.ui->actionDisconnect->isEnabled());
    QVERIFY(!w.ui->actionReset->isEnabled());
}

void TstMainWindow::toolbarHasReaderCombo()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    w.show();
    QVERIFY(w.cardReaderCombo != nullptr);
    QVERIFY(w.cardReaderCombo->minimumWidth() >= 300);
    QVERIFY(w.ui->toolBar->isAncestorOf(w.cardReaderCombo));
}

void TstMainWindow::logDockToggleFollowsVisibility()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    w.show();
    QVERIFY(w.ui->dockWidget->isVisible());
    QVERIFY(w.ui->actionLogWindow->isChecked());

    w.ui->actionLogWindow->trigger();
    QVERIFY(!w.ui->dockWidget->isVisible());
    QVERIFY(!w.ui->actionLogWindow->isChecked());

    w.ui->actionLogWindow->trigger();
    QVERIFY(w.ui->dockWidget->isVisible());
}

void TstMainWindow::logMessagesAppearInLogView()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    w.show();
    auto *model = w.ui->logView->model();
    const int before = model->rowCount();

    w.logString(LogType::Info, "hello");
    w.logBytes(LogType::CardCmd, QByteArray::fromHex("00A4"));

    QCOMPARE(model->rowCount(), before + 2);
    QCOMPARE(model->index(before, 1).data().toString(), QString("hello"));
    QCOMPARE(model->index(before + 1, 1).data().toString(), QString("00 A4"));
}

void TstMainWindow::sendCommandTabSignalsAreWired()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    auto *model = w.ui->logView->model();
    const int before = model->rowCount();

    emit w.tabSendCommand->logString(LogType::Info, "from send tab");
    emit w.tabSendCommand->logBytes(LogType::CardCmd, QByteArray::fromHex("00"));

    QCOMPARE(model->rowCount(), before + 2);
}

void TstMainWindow::runScriptTabSignalsAreWired()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    auto *model = w.ui->logView->model();
    const int before = model->rowCount();

    emit w.tabRunScript->logString(LogType::Error, "from run script tab");

    QCOMPARE(model->rowCount(), before + 1);
}

void TstMainWindow::exitActionClosesWindow()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    w.ui->actionExit->trigger();
    QVERIFY(!w.isVisible());
}

void TstMainWindow::readersAreListedInTheBackground()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    w.show();

    QTRY_COMPARE(w.cardReaderCombo->count(), 2);
    QCOMPARE(session.reader(), QString("Mock Reader 0"));

    w.cardReaderCombo->setCurrentIndex(1);
    QCOMPARE(session.reader(), QString("Mock Reader 1"));
}

void TstMainWindow::connectDisconnectResetRunInTheBackground()
{
    MockTransport transport;
    transport.transmitDelayMs = 0;
    CardSession session(&transport);
    MainWindow w(&session);
    w.show();
    QTRY_COMPARE(w.cardReaderCombo->count(), 2);

    w.ui->actionConnect->trigger();
    QVERIFY(!w.ui->actionConnect->isEnabled());     // busy until the worker is done
    QTRY_VERIFY(session.isConnected());
    QTRY_VERIFY(w.ui->actionDisconnect->isEnabled());
    QVERIFY(w.ui->actionReset->isEnabled());
    QVERIFY(!w.ui->actionConnect->isEnabled());

    w.ui->actionReset->trigger();
    QTRY_COMPARE(transport.resetCount, 1);
    QTRY_VERIFY(w.ui->actionReset->isEnabled());

    w.ui->actionDisconnect->trigger();
    QTRY_VERIFY(!session.isConnected());
    QTRY_VERIFY(w.ui->actionConnect->isEnabled());
}

void TstMainWindow::cardRemovalSwitchesToDisconnectedState()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    w.show();
    QTRY_COMPARE(w.cardReaderCombo->count(), 2);
    w.ui->actionConnect->trigger();
    QTRY_VERIFY(w.ui->actionDisconnect->isEnabled());
    QVERIFY(w.statusLine->text().startsWith("Connected"));

    transport.cardInserted = false;

    QTRY_VERIFY(w.ui->actionConnect->isEnabled());
    QVERIFY(!w.ui->actionDisconnect->isEnabled());
    QVERIFY(!w.ui->actionReset->isEnabled());
    QVERIFY(w.statusLine->text().startsWith("Ready"));
    // the removal is reported in the log view
    auto *model = w.ui->logView->model();
    bool found = false;
    for (int i = 0; i < model->rowCount(); ++i)
        found = found || model->index(i, 1).data().toString().contains("Card removed");
    QVERIFY(found);
}

void TstMainWindow::atrTabIsShownWhileConnected()
{
    MockTransport transport;
    CardSession session(&transport);
    MainWindow w(&session);
    w.show();
    QTRY_COMPARE(w.cardReaderCombo->count(), 2);

    auto *tabs = w.ui->tabWidget;
    auto *atrTab = w.findChild<AtrView *>("tabAtr");
    QVERIFY(atrTab);
    QCOMPARE(tabs->tabText(tabs->indexOf(atrTab)), QString("ATR"));
    QVERIFY(!tabs->isTabVisible(tabs->indexOf(atrTab)));
    QCOMPARE(tabs->indexOf(atrTab), tabs->count() - 1);   // right most

    w.ui->actionConnect->trigger();
    QTRY_VERIFY(tabs->isTabVisible(tabs->indexOf(atrTab)));
    QCOMPARE(atrTab->atr(), transport.cardAtr);

    transport.cardInserted = false;
    QTRY_VERIFY(!tabs->isTabVisible(tabs->indexOf(atrTab)));
}

static TestRegistrar<TstMainWindow> registrar;
#include "tst_mainwindow.moc"
