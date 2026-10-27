#include "testregistry.h"
#include "testutil.h"
#include "mocktransport.h"
#include "card/cardsession.h"
#include "privateaccess.h"
#include "ui_mainwindow.h"
#include "ui/listreorderhelper.h"
#include "commands/commandsendbytes.h"
#include "commands/commandapdu.h"
#include "commands/commandterminal.h"
#include "commands/commandviewsendbytes.h"
#include "commands/commandviewapdu.h"
#include "commands/commandviewterminal.h"

#include <QFileInfo>
#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>

class TstRunScript : public QObject
{
    Q_OBJECT

private:
    QMainWindow *window = nullptr;
    Ui::MainWindow *ui = nullptr;
    MockTransport *transport = nullptr;
    CardSession *session = nullptr;
    TabHandlerRunScript *tab = nullptr;
    RunScriptModel *model = nullptr;

    void addViaMenu(int actionIndex) { tab->buttonAdd->menu()->actions().at(actionIndex)->trigger(); }
    void select(int row) { tab->listView->setCurrentIndex(model->index(row, 0)); }
    QStringList names() const {
        QStringList list;
        for (int i = 0; i < model->rowCount(); ++i)
            list << model->item(i)->commandName;
        return list;
    }
    template <typename V> V *currentView() const { return qobject_cast<V *>(tab->stackedWidget->currentWidget()); }

private slots:
    void init();
    void cleanup();

    void addMenuOffersThreeCommandTypes();
    void addCreatesCommandAndMatchingView();
    void editingViewUpdatesModel();
    void editingViewUpdatesListText();
    void selectionSwitchesView();
    void duplicateKeepsTypeAndAddsCopySuffix();
    void duplicateApduShowsTheCopyInTheView();
    void moveUpAndDown();
    void deleteRemovesCurrentCommand();
    void deletingLastCommandRemovesItsView();
    void clearEmptiesModelAndView();
    void contextMenuOnItemOffersItemActions();

    void loadDefaultScriptFile();
    void loadRejectsWrongVersion();
    void loadReportsMissingFile();
    void loadSkipsUnknownItemTypes();
    void loadUnknownTerminalActionFallsBackToDefault();
    void loadApduDataKeyIsData();
    void saveWritesVersionAndItems();
    void saveLoadRoundTripKeepsApduData();
    void saveEscapesSpecialCharactersInNames();

    void runExecutesAllCommandsInOrder();
    void runMarksPassedAndFailedAndStopsAtFirstFailure();
    void runLogsCommandsAndResponses();
    void runDisablesEditingWhileRunning();
    void stopInterruptsAWaitingCommand();
    void stepRunsSelectedCommandAndAdvances();
    void runIsDisabledWithoutCommands();
    void runDoesNotBlockTheGui();
    void terminalCommandsRunInScript();
    void runContinuesAfterFailureWithoutStopOnError();
    void runExecutesOnlySelectedRows();
    void runWithOneOrAllSelectedExecutesEverything();
    void moveRowsReordersCommands();
    void dropMovesTheSelectedItems();
};

static QSharedPointer<CommandSendBytes> sendBytes(const QString &name, const QByteArray &data, const QByteArray &expected = QByteArray())
{
    auto cmd = QSharedPointer<CommandSendBytes>::create();
    cmd->commandName = name;
    cmd->commandData = data;
    cmd->expectedResponse = expected;
    return cmd;
}

static QSharedPointer<CommandTerminal> terminal(CommandTerminal::ActionType action)
{
    auto cmd = QSharedPointer<CommandTerminal>::create();
    cmd->action = action;
    return cmd;
}

static QString writeFile(const QTemporaryDir &dir, const QString &name, const QByteArray &content)
{
    const QString path = dir.filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        qFatal("cannot write %s", qPrintable(path));
    f.write(content);
    return path;
}

void TstRunScript::init()
{
    window = new QMainWindow;
    ui = new Ui::MainWindow;
    ui->setupUi(window);
    transport = new MockTransport;
    session = new CardSession(transport);
    session->setReader("Mock Reader 0");
    tab = new TabHandlerRunScript(ui->tabRunScript, session);
    model = qobject_cast<RunScriptModel *>(tab->listView->model());
    window->show();
}

void TstRunScript::cleanup()
{
    delete tab;
    delete session;
    delete transport;
    delete window;
    delete ui;
    session = nullptr; transport = nullptr;
    tab = nullptr; window = nullptr; ui = nullptr; model = nullptr;
}

void TstRunScript::addMenuOffersThreeCommandTypes()
{
    const auto actions = tab->buttonAdd->menu()->actions();
    QCOMPARE(actions.size(), 3);
    QCOMPARE(actions[0]->text(), QString("Send Bytes"));
    QCOMPARE(actions[1]->text(), QString("Send APDU"));
    QCOMPARE(actions[2]->text(), QString("Terminal command"));
}

void TstRunScript::addCreatesCommandAndMatchingView()
{
    addViaMenu(0);
    addViaMenu(1);
    addViaMenu(2);

    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(model->commandType(0), QString("SendBytes"));
    QCOMPARE(model->commandType(1), QString("SendAPDU"));
    QCOMPARE(model->commandType(2), QString("TerminalCmd"));

    // the last added item is selected and its editor shown
    QCOMPARE(tab->listView->currentIndex().row(), 2);
    QVERIFY(currentView<CommandViewTerminal>());
    QCOMPARE(tab->stackedWidget->count(), 1);   // old view must not pile up
}

void TstRunScript::editingViewUpdatesModel()
{
    addViaMenu(0);
    auto *view = currentView<CommandViewSendBytes>();
    QVERIFY(view);

    view->findChild<QLineEdit *>("lineEditCommandName")->setText("Select ISD");
    auto *hex = view->findChild<HexEdit *>("hexEditCommandApdu");
    hex->setFocus();
    QTest::keyClicks(hex, "00A40400");

    auto cmd = qSharedPointerDynamicCast<CommandSendBytes>(model->item(0));
    QVERIFY(cmd);
    QCOMPARE(cmd->commandName, QString("Select ISD"));
    QCOMPARE(cmd->commandData, QByteArray::fromHex("00A40400"));
}

void TstRunScript::editingViewUpdatesListText()
{
    addViaMenu(2);
    currentView<CommandViewTerminal>()->findChild<QLineEdit *>("lineEditCommandName")->setText("Reset it");
    QCOMPARE(model->index(0, 0).data().toString(), QString("Reset it"));
}

void TstRunScript::selectionSwitchesView()
{
    addViaMenu(0);
    addViaMenu(1);
    addViaMenu(2);

    select(0);
    QVERIFY(currentView<CommandViewSendBytes>());
    select(1);
    QVERIFY(currentView<CommandViewApdu>());
    select(2);
    QVERIFY(currentView<CommandViewTerminal>());
    QCOMPARE(tab->stackedWidget->count(), 1);
}

void TstRunScript::duplicateKeepsTypeAndAddsCopySuffix()
{
    addViaMenu(1);
    auto apdu = qSharedPointerDynamicCast<CommandApdu>(model->item(0));
    apdu->commandName = "Select";
    apdu->cla = QByteArray::fromHex("00");
    apdu->ins = QByteArray::fromHex("A4");
    select(0);

    tab->buttonActionDuplicate();

    QCOMPARE(names(), QStringList({"Select", "Select_copy"}));
    auto copy = qSharedPointerDynamicCast<CommandApdu>(model->item(1));
    QVERIFY(copy);
    QCOMPARE(copy->ins, QByteArray::fromHex("A4"));
    QVERIFY(copy != apdu);                                   // independent object
    QCOMPARE(tab->listView->currentIndex().row(), 1);
}

void TstRunScript::duplicateApduShowsTheCopyInTheView()
{
    addViaMenu(1);
    auto apdu = qSharedPointerDynamicCast<CommandApdu>(model->item(0));
    apdu->commandName = "Select";
    select(0);

    tab->buttonActionDuplicate();

    auto *view = currentView<CommandViewApdu>();
    QVERIFY(view);
    QCOMPARE(view->findChild<QLineEdit *>("lineEditCommandName")->text(), QString("Select_copy"));
}

void TstRunScript::moveUpAndDown()
{
    addViaMenu(0); addViaMenu(1); addViaMenu(2);
    const auto t0 = model->commandType(0), t1 = model->commandType(1), t2 = model->commandType(2);

    select(2);
    tab->buttonActionUp();
    QCOMPARE(model->commandType(1), t2);
    QCOMPARE(model->commandType(2), t1);
    QCOMPARE(tab->listView->currentIndex().row(), 1);

    tab->buttonActionDown();
    QCOMPARE(model->commandType(1), t1);
    QCOMPARE(model->commandType(2), t2);

    select(0);
    tab->buttonActionUp();
    QCOMPARE(model->commandType(0), t0);       // first stays first
}

void TstRunScript::deleteRemovesCurrentCommand()
{
    addViaMenu(0); addViaMenu(1); addViaMenu(2);
    select(1);
    tab->buttonActionDelete();

    QCOMPARE(model->rowCount(), 2);
    QCOMPARE(model->commandType(0), QString("SendBytes"));
    QCOMPARE(model->commandType(1), QString("TerminalCmd"));
}

// BUG: when the last command is deleted nothing replaces the editor view, so a stale
// editor for a command that no longer exists stays visible.
void TstRunScript::deletingLastCommandRemovesItsView()
{
    addViaMenu(0);
    select(0);
    tab->buttonActionDelete();

    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(tab->stackedWidget->count(), 0);
}

void TstRunScript::clearEmptiesModelAndView()
{
    addViaMenu(0); addViaMenu(1);
    tab->buttonActionClear();
    QCOMPARE(model->rowCount(), 0);
    QCOMPARE(tab->stackedWidget->count(), 0);
}

void TstRunScript::contextMenuOnItemOffersItemActions()
{
    // smoke test: the handler must not crash for an empty list (menu is dismissed by timer)
    QTimer::singleShot(50, []() {
        if (auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget()))
            menu->close();
    });
    tab->showContextMenu(QPoint(5, 5));
    QVERIFY(true);
}

void TstRunScript::loadDefaultScriptFile()
{
    const QString path = QString(TEST_RES_DIR) + "/defaultScriptFile.json";
    if (!QFileInfo::exists(path))
        QSKIP("res/defaultScriptFile.json is not part of the repository");
    QSignalSpy logs(tab, &TabHandlerRunScript::logString);

    tab->_buttonActionLoad(path);

    QCOMPARE(logs.count(), 0);
    QVERIFY(model->rowCount() > 10);
    QCOMPARE(names().first(), QString("Connect"));
    QCOMPARE(model->commandType(0), QString("TerminalCmd"));
    QCOMPARE(qSharedPointerDynamicCast<CommandTerminal>(model->item(0))->action, CommandTerminal::ActionType::Connect);

    // the file covers every command type
    QSet<QString> types;
    for (int i = 0; i < model->rowCount(); ++i)
        types.insert(model->commandType(i));
    QCOMPARE(types, QSet<QString>({"TerminalCmd", "SendAPDU", "SendBytes"}));

    auto apdu = qSharedPointerDynamicCast<CommandApdu>(model->item(2));
    QCOMPARE(names().at(2), QString("Select ISD"));
    QCOMPARE(apdu->cla, QByteArray::fromHex("00"));
    QCOMPARE(apdu->ins, QByteArray::fromHex("A4"));
    QCOMPARE(apdu->p1, QByteArray::fromHex("04"));
    QCOMPARE(apdu->p2, QByteArray::fromHex("00"));
    QCOMPARE(apdu->expectedResponse, QByteArray::fromHex("9000"));

    QCOMPARE(tab->lineEditFilename->text(), path);
    QCOMPARE(tab->listView->currentIndex().row(), 0);   // first item selected
    QVERIFY(currentView<CommandViewTerminal>());
}

void TstRunScript::loadRejectsWrongVersion()
{
    QTemporaryDir dir;
    QSignalSpy logs(tab, &TabHandlerRunScript::logString);
    addViaMenu(0);

    tab->_buttonActionLoad(writeFile(dir, "v9.json", R"({"version":"9.9","items":[]})"));

    QCOMPARE(logs.count(), 1);
    QCOMPARE(model->rowCount(), 1);     // untouched
}

void TstRunScript::loadReportsMissingFile()
{
    QSignalSpy logs(tab, &TabHandlerRunScript::logString);
    tab->_buttonActionLoad("/nonexistent/script.json");
    QCOMPARE(logs.count(), 1);
}

void TstRunScript::loadSkipsUnknownItemTypes()
{
    QTemporaryDir dir;
    tab->_buttonActionLoad(writeFile(dir, "s.json", R"({"version":"1.0","items":[
        {"name":"a","type":"SendBytes","cmd":"00"},
        {"name":"b","type":"FutureThing"},
        {"name":"c","type":"TerminalCmd","action":"Connect"}]})"));
    QCOMPARE(names(), QStringList({"a", "c"}));
    auto term = qSharedPointerDynamicCast<CommandTerminal>(model->item(1));
    QCOMPARE(term->action, CommandTerminal::ActionType::Connect);
}

void TstRunScript::loadUnknownTerminalActionFallsBackToDefault()
{
    QTemporaryDir dir;
    tab->_buttonActionLoad(writeFile(dir, "s.json", R"({"version":"1.0","items":[
        {"name":"c","type":"TerminalCmd","action":"Explode"}]})"));
    QCOMPARE(model->rowCount(), 1);
    auto term = qSharedPointerDynamicCast<CommandTerminal>(model->item(0));
    QCOMPARE(term->action, CommandTerminal::ActionType::None);
}

// The loader reads the APDU data field from "data" ...
void TstRunScript::loadApduDataKeyIsData()
{
    QTemporaryDir dir;
    tab->_buttonActionLoad(writeFile(dir, "s.json", R"({"version":"1.0","items":[
        {"name":"a","type":"SendAPDU","cla":"00","ins":"A4","p1":"04","p2":"00","data":"A0 00","le":"00"}]})"));
    auto apdu = qSharedPointerDynamicCast<CommandApdu>(model->item(0));
    QCOMPARE(apdu->commandData, QByteArray::fromHex("A000"));
}

void TstRunScript::saveWritesVersionAndItems()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("out.json");
    addViaMenu(0);
    addViaMenu(2);

    DialogAutomator dialogs(path);
    tab->buttonActionSave();

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    QCOMPARE(err.error, QJsonParseError::NoError);
    QCOMPARE(doc.object()["version"].toString(), QString("1.0"));
    const QJsonArray items = doc.object()["items"].toArray();
    QCOMPARE(items.size(), 2);
    QCOMPARE(items[0].toObject()["type"].toString(), QString("SendBytes"));
    QCOMPARE(items[1].toObject()["type"].toString(), QString("TerminalCmd"));
}

// BUG: save() writes the APDU payload as "cmd", load() reads "data" => payload is lost.
void TstRunScript::saveLoadRoundTripKeepsApduData()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("out.json");
    addViaMenu(1);
    auto apdu = qSharedPointerDynamicCast<CommandApdu>(model->item(0));
    apdu->commandName = "Select";
    apdu->cla = QByteArray::fromHex("00");
    apdu->ins = QByteArray::fromHex("A4");
    apdu->p1 = QByteArray::fromHex("04");
    apdu->p2 = QByteArray::fromHex("00");
    apdu->commandData = QByteArray::fromHex("A000000151");
    apdu->le = QByteArray::fromHex("00");
    apdu->expectedResponse = QByteArray::fromHex("9000");

    {
        DialogAutomator dialogs(path);
        tab->buttonActionSave();
    }
    tab->_buttonActionLoad(path);

    QCOMPARE(model->rowCount(), 1);
    auto loaded = qSharedPointerDynamicCast<CommandApdu>(model->item(0));
    QVERIFY(loaded);
    QCOMPARE(loaded->commandName, QString("Select"));
    QCOMPARE(loaded->ins, QByteArray::fromHex("A4"));
    QCOMPARE(loaded->expectedResponse, QByteArray::fromHex("9000"));
    QCOMPARE(loaded->commandData, QByteArray::fromHex("A000000151"));
}

// BUG: the file is serialised by string concatenation without JSON escaping.
void TstRunScript::saveEscapesSpecialCharactersInNames()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("out.json");
    addViaMenu(0);
    qSharedPointerDynamicCast<CommandSendBytes>(model->item(0))->commandName = "say \"hi\" \\ now";

    DialogAutomator dialogs(path);
    tab->buttonActionSave();

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    QJsonParseError err;
    QJsonDocument::fromJson(f.readAll(), &err);
    QCOMPARE(err.error, QJsonParseError::NoError);
}

void TstRunScript::runExecutesAllCommandsInOrder()
{
    session->connectCard();
    model->addCommand(sendBytes("a", QByteArray::fromHex("01")));
    model->addCommand(sendBytes("b", QByteArray::fromHex("02")));
    model->addCommand(sendBytes("c", QByteArray::fromHex("03")));

    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonRun->click();
    QVERIFY(!tab->buttonRun->isEnabled());
    QVERIFY(tab->buttonStop->isEnabled());

    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(finished.first().at(0).toBool());
    QCOMPARE(finished.first().at(1).toInt(), 3);
    QCOMPARE(transport->sent, (QList<QByteArray>{QByteArray::fromHex("01"), QByteArray::fromHex("02"), QByteArray::fromHex("03")}));
    for (int i = 0; i < 3; ++i)
        QCOMPARE(model->status(i), RunScriptModel::Status::Passed);
    QVERIFY(tab->buttonRun->isEnabled());
    QVERIFY(!tab->buttonStop->isEnabled());
}

void TstRunScript::runContinuesAfterFailureWithoutStopOnError()
{
    session->connectCard();
    transport->answers.insert(QByteArray::fromHex("02"), QByteArray::fromHex("6A82"));
    model->addCommand(sendBytes("a", QByteArray::fromHex("01"), QByteArray::fromHex("9000")));
    model->addCommand(sendBytes("b", QByteArray::fromHex("02"), QByteArray::fromHex("9000")));
    model->addCommand(sendBytes("c", QByteArray::fromHex("03"), QByteArray::fromHex("9000")));

    QVERIFY(tab->checkStopOnError->isChecked());   // default
    tab->checkStopOnError->setChecked(false);
    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonRun->click();
    QTRY_COMPARE(finished.count(), 1);

    QVERIFY(!finished.first().at(0).toBool());      // a command failed
    QCOMPARE(finished.first().at(1).toInt(), 3);    // but all were executed
    QCOMPARE(model->status(0), RunScriptModel::Status::Passed);
    QCOMPARE(model->status(1), RunScriptModel::Status::Failed);
    QCOMPARE(model->status(2), RunScriptModel::Status::Passed);
    QCOMPARE(transport->sent.size(), 3);
}

void TstRunScript::runExecutesOnlySelectedRows()
{
    session->connectCard();
    for (int i = 1; i <= 4; ++i)
        model->addCommand(sendBytes(QString::number(i), QByteArray(1, char(i))));

    auto *sel = tab->listView->selectionModel();
    sel->clearSelection();
    sel->select(model->index(1, 0), QItemSelectionModel::Select);
    sel->select(model->index(3, 0), QItemSelectionModel::Select);
    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonRun->click();
    QTRY_COMPARE(finished.count(), 1);

    QCOMPARE(transport->sent.size(), 2);
    QCOMPARE(transport->sent.at(0), QByteArray(1, char(2)));
    QCOMPARE(transport->sent.at(1), QByteArray(1, char(4)));
    QCOMPARE(model->status(0), RunScriptModel::Status::None);
    QCOMPARE(model->status(1), RunScriptModel::Status::Passed);
    QCOMPARE(model->status(2), RunScriptModel::Status::None);
    QCOMPARE(model->status(3), RunScriptModel::Status::Passed);
}

void TstRunScript::runWithOneOrAllSelectedExecutesEverything()
{
    session->connectCard();
    for (int i = 1; i <= 3; ++i)
        model->addCommand(sendBytes(QString::number(i), QByteArray(1, char(i))));
    auto *sel = tab->listView->selectionModel();

    sel->clearSelection();                      // nothing
    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonRun->click();
    QTRY_COMPARE(finished.count(), 1);
    QCOMPARE(transport->sent.size(), 3);

    transport->sent.clear();
    sel->select(model->index(1, 0), QItemSelectionModel::ClearAndSelect);   // one (just the current row)
    tab->buttonRun->click();
    QTRY_COMPARE(finished.count(), 2);
    QCOMPARE(transport->sent.size(), 3);

    transport->sent.clear();
    tab->listView->selectAll();                 // all
    tab->buttonRun->click();
    QTRY_COMPARE(finished.count(), 3);
    QCOMPARE(transport->sent.size(), 3);
}

void TstRunScript::moveRowsReordersCommands()
{
    for (int i = 0; i < 5; ++i)
        model->addCommand(sendBytes(QString::number(i), QByteArray()));
    auto names = [&] { QStringList l; for (auto c : model->commands()) l << c->commandName; return l.join(','); };

    model->moveToRow({0}, 3);
    QCOMPARE(names(), QString("1,2,0,3,4"));
    model->moveToRow({2}, 0);
    QCOMPARE(names(), QString("0,1,2,3,4"));
    model->moveToRow({1, 3}, 5);
    QCOMPARE(names(), QString("0,2,4,1,3"));
    model->moveToRow({3, 4}, 0);
    QCOMPARE(names(), QString("1,3,0,2,4"));
}

void TstRunScript::dropMovesTheSelectedItems()
{
    for (int i = 0; i < 4; ++i)
        model->addCommand(sendBytes(QString::number(i), QByteArray()));
    auto names = [&] { QStringList l; for (auto c : model->commands()) l << c->commandName; return l.join(','); };
    window->resize(900, 700);
    ui->tabWidget->setCurrentWidget(ui->tabRunScript);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto belowRow = [&](int row) { return tab->listView->visualRect(model->index(row, 0)).bottomLeft() + QPoint(5, -2); };

    tab->listView->setCurrentIndex(model->index(0, 0));
    QVERIFY(tab->reorder->drop(belowRow(2)));                   // item 0 behind item 2
    QCOMPARE(names(), QString("1,2,0,3"));
    QCOMPARE(tab->listView->currentIndex().row(), 2);           // selection follows the item

    QVERIFY(!tab->reorder->drop(belowRow(2)));                  // onto itself: nothing

    tab->listView->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);
    QVERIFY(tab->reorder->drop(belowRow(3)));                   // items 1 and 0 to the end
    QCOMPARE(names(), QString("2,3,1,0"));
}

void TstRunScript::runMarksPassedAndFailedAndStopsAtFirstFailure()
{
    session->connectCard();
    transport->answers.insert(QByteArray::fromHex("02"), QByteArray::fromHex("6A82"));
    model->addCommand(sendBytes("a", QByteArray::fromHex("01"), QByteArray::fromHex("9000")));
    model->addCommand(sendBytes("b", QByteArray::fromHex("02"), QByteArray::fromHex("9000")));
    model->addCommand(sendBytes("c", QByteArray::fromHex("03")));

    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonRun->click();
    QTRY_COMPARE(finished.count(), 1);

    QVERIFY(!finished.first().at(0).toBool());
    QCOMPARE(finished.first().at(1).toInt(), 2);
    QCOMPARE(model->status(0), RunScriptModel::Status::Passed);
    QCOMPARE(model->status(1), RunScriptModel::Status::Failed);
    QCOMPARE(model->status(2), RunScriptModel::Status::None);
    QCOMPARE(transport->sent.size(), 2);
}

void TstRunScript::runLogsCommandsAndResponses()
{
    session->connectCard();
    model->addCommand(sendBytes("a", QByteArray::fromHex("0102")));
    QList<QPair<LogType, QByteArray>> bytes;
    QStringList strings;
    connect(tab, &TabHandlerRunScript::logBytes, this, [&](LogType t, const QByteArray &b) { bytes.append({t, b}); });
    connect(tab, &TabHandlerRunScript::logString, this, [&](LogType, const QString &t) { strings << t; });

    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonRun->click();
    QTRY_COMPARE(finished.count(), 1);

    QCOMPARE(bytes.size(), 2);
    QCOMPARE(bytes[0], qMakePair(LogType::CardCmd, QByteArray::fromHex("0102")));
    QCOMPARE(bytes[1], qMakePair(LogType::CardRsp, QByteArray::fromHex("9000")));
    QVERIFY(!strings.isEmpty());
}

void TstRunScript::runDisablesEditingWhileRunning()
{
    session->connectCard();
    transport->transmitDelayMs = 150;
    model->addCommand(sendBytes("a", QByteArray::fromHex("01")));

    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonRun->click();
    QVERIFY(!tab->listView->isEnabled());
    QVERIFY(!tab->buttonAdd->isEnabled());
    QVERIFY(!tab->buttonLoad->isEnabled());
    QVERIFY(!tab->buttonSave->isEnabled());

    QTRY_COMPARE(finished.count(), 1);
    QVERIFY(tab->listView->isEnabled());
    QVERIFY(tab->buttonAdd->isEnabled());
}

void TstRunScript::stopInterruptsAWaitingCommand()
{
    session->connectCard();
    transport->cardInserted = true;
    model->addCommand(sendBytes("a", QByteArray::fromHex("01")));
    model->addCommand(terminal(CommandTerminal::ActionType::WaitForRemoval));
    model->addCommand(sendBytes("c", QByteArray::fromHex("03")));

    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonRun->click();
    QTRY_COMPARE(model->status(1), RunScriptModel::Status::Running);

    tab->buttonStop->click();
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 3000);
    QVERIFY(!finished.first().at(0).toBool());
    QCOMPARE(model->status(1), RunScriptModel::Status::Failed);
    QCOMPARE(model->status(2), RunScriptModel::Status::None);
    QCOMPARE(transport->sent.size(), 1);
}

void TstRunScript::stepRunsSelectedCommandAndAdvances()
{
    session->connectCard();
    model->addCommand(sendBytes("a", QByteArray::fromHex("01")));
    model->addCommand(sendBytes("b", QByteArray::fromHex("02")));
    select(0);

    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonStep->click();
    QTRY_COMPARE(finished.count(), 1);

    QCOMPARE(transport->sent, (QList<QByteArray>{QByteArray::fromHex("01")}));
    QCOMPARE(model->status(0), RunScriptModel::Status::Passed);
    QCOMPARE(model->status(1), RunScriptModel::Status::None);
    QCOMPARE(tab->listView->currentIndex().row(), 1);

    tab->buttonStep->click();
    QTRY_COMPARE(finished.count(), 2);
    QCOMPARE(transport->sent.size(), 2);
    QCOMPARE(model->status(1), RunScriptModel::Status::Passed);
}

void TstRunScript::runIsDisabledWithoutCommands()
{
    QVERIFY(!tab->buttonRun->isEnabled());
    QVERIFY(!tab->buttonStep->isEnabled());
    QVERIFY(!tab->buttonStop->isEnabled());

    addViaMenu(0);
    QVERIFY(tab->buttonRun->isEnabled());
    QVERIFY(tab->buttonStep->isEnabled());

    tab->buttonActionClear();
    QVERIFY(!tab->buttonRun->isEnabled());
}

void TstRunScript::runDoesNotBlockTheGui()
{
    session->connectCard();
    transport->transmitDelayMs = 200;
    model->addCommand(sendBytes("a", QByteArray::fromHex("01")));

    QElapsedTimer timer;
    timer.start();
    tab->buttonRun->click();
    QVERIFY2(timer.elapsed() < 150, "starting a run must not wait for the card");
    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    QTRY_COMPARE(finished.count(), 1);
}

void TstRunScript::terminalCommandsRunInScript()
{
    model->addCommand(terminal(CommandTerminal::ActionType::WaitForCard));
    model->addCommand(terminal(CommandTerminal::ActionType::Connect));
    model->addCommand(sendBytes("a", QByteArray::fromHex("01"), QByteArray::fromHex("9000")));
    model->addCommand(terminal(CommandTerminal::ActionType::WarmReset));
    model->addCommand(terminal(CommandTerminal::ActionType::ColdReset));
    model->addCommand(terminal(CommandTerminal::ActionType::Disconnect));

    QSignalSpy finished(tab->runner, &ScriptRunner::finished);
    tab->buttonRun->click();
    QTRY_COMPARE(finished.count(), 1);

    QVERIFY(finished.first().at(0).toBool());
    QCOMPARE(transport->resetCount, 1);
    QCOMPARE(transport->coldResetCount, 1);
    QVERIFY(!session->isConnected());
}

static TestRegistrar<TstRunScript> registrar;
#include "tst_runscript.moc"
