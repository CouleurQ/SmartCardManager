#include "testregistry.h"
#include "privateaccess.h"
#include "ui_mainwindow.h"

#include <QTest>
#include <QSignalSpy>

// BUG B-17: destroying a visible MainWindow aborts (dock visibilityChanged fires into the
// half-destroyed window), so tests hide it first.
struct HideGuard
{
    QWidget *w;
    ~HideGuard() { w->hide(); }
};

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
};

void TstMainWindow::startsDisconnected()
{
    MainWindow w;
    HideGuard guard{&w};
    w.show();
    QVERIFY(w.ui->actionConnect->isEnabled() == !w.app->pcsc->isConnected(QString()));
    QVERIFY(!w.ui->actionDisconnect->isEnabled());
    QVERIFY(!w.ui->actionReset->isEnabled());
}

void TstMainWindow::toolbarHasReaderCombo()
{
    MainWindow w;
    HideGuard guard{&w};
    w.show();
    QVERIFY(w.cardReaderCombo != nullptr);
    QVERIFY(w.cardReaderCombo->minimumWidth() >= 300);
    QVERIFY(w.ui->toolBar->isAncestorOf(w.cardReaderCombo));
}

void TstMainWindow::logDockToggleFollowsVisibility()
{
    MainWindow w;
    HideGuard guard{&w};
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
    MainWindow w;
    HideGuard guard{&w};
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
    MainWindow w;
    HideGuard guard{&w};
    auto *model = w.ui->logView->model();
    const int before = model->rowCount();

    emit w.tabSendCommand->logString(LogType::Info, "from send tab");
    emit w.tabSendCommand->logBytes(LogType::CardCmd, QByteArray::fromHex("00"));

    QCOMPARE(model->rowCount(), before + 2);
}

// BUG: MainWindow never connects TabHandlerRunScript's signals, so e.g. load errors vanish.
void TstMainWindow::runScriptTabSignalsAreWired()
{
    MainWindow w;
    HideGuard guard{&w};
    auto *model = w.ui->logView->model();
    const int before = model->rowCount();

    emit w.tabRunScript->logString(LogType::Error, "from run script tab");

    QEXPECT_FAIL("", "RunScript logString/logBytes are not connected, CODE_REVIEW B-14", Continue);
    QCOMPARE(model->rowCount(), before + 1);
}

void TstMainWindow::exitActionClosesWindow()
{
    MainWindow w;
    HideGuard guard{&w};
    w.show();
    QVERIFY(QTest::qWaitForWindowExposed(&w));
    w.ui->actionExit->trigger();
    QVERIFY(!w.isVisible());
}

static TestRegistrar<TstMainWindow> registrar;
#include "tst_mainwindow.moc"
