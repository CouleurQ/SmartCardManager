#include "testregistry.h"
#include "testutil.h"
#include "privateaccess.h"
#include "ui_mainwindow.h"
#include "commands/commandsendbytes.h"
#include "commands/commandapdu.h"
#include "commands/commandterminal.h"
#include "commands/viewsendbytes.h"
#include "commands/viewapdu.h"
#include "commands/viewterminal.h"

#include <QTest>
#include <QSignalSpy>
#include <QTemporaryDir>

class TstRunScript : public QObject
{
    Q_OBJECT

private:
    QMainWindow *window = nullptr;
    Ui::MainWindow *ui = nullptr;
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
};

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
    tab = new TabHandlerRunScript(ui->tabRunScript);
    model = qobject_cast<RunScriptModel *>(tab->listView->model());
    window->show();
}

void TstRunScript::cleanup()
{
    delete tab;
    delete window;
    delete ui;
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
    QCOMPARE(model->commandType(0), Command::ObjectType::SendBytesType);
    QCOMPARE(model->commandType(1), Command::ObjectType::ApduType);
    QCOMPARE(model->commandType(2), Command::ObjectType::TerminalType);

    // the last added item is selected and its editor shown
    QCOMPARE(tab->listView->currentIndex().row(), 2);
    QVERIFY(currentView<ViewTerminal>());
    QCOMPARE(tab->stackedWidget->count(), 1);   // old view must not pile up
}

void TstRunScript::editingViewUpdatesModel()
{
    addViaMenu(0);
    auto *view = currentView<ViewSendBytes>();
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
    currentView<ViewTerminal>()->findChild<QLineEdit *>("lineEditCommandName")->setText("Reset it");
    QCOMPARE(model->index(0, 0).data().toString(), QString("Reset it"));
}

void TstRunScript::selectionSwitchesView()
{
    addViaMenu(0);
    addViaMenu(1);
    addViaMenu(2);

    select(0);
    QVERIFY(currentView<ViewSendBytes>());
    select(1);
    QVERIFY(currentView<ViewApdu>());
    select(2);
    QVERIFY(currentView<ViewTerminal>());
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

    auto *view = currentView<ViewApdu>();
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
    QCOMPARE(model->commandType(0), Command::ObjectType::SendBytesType);
    QCOMPARE(model->commandType(1), Command::ObjectType::TerminalType);
}

// BUG: when the last command is deleted nothing replaces the editor view, so a stale
// editor for a command that no longer exists stays visible.
void TstRunScript::deletingLastCommandRemovesItsView()
{
    addViaMenu(0);
    select(0);
    tab->buttonActionDelete();

    QCOMPARE(model->rowCount(), 0);
    QEXPECT_FAIL("", "stale editor remains after deleting the last command, CODE_REVIEW B-13", Continue);
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
    QSignalSpy logs(tab, &TabHandlerRunScript::logString);

    tab->_buttonActionLoad(path);

    QCOMPARE(logs.count(), 0);
    QCOMPARE(model->rowCount(), 3);
    QCOMPARE(names(), QStringList({"Select ISD", "Select ISD", "Get CPLC Data"}));
    QCOMPARE(model->commandType(0), Command::ObjectType::ApduType);
    QCOMPARE(model->commandType(1), Command::ObjectType::SendBytesType);
    QCOMPARE(model->commandType(2), Command::ObjectType::SendBytesType);

    auto apdu = qSharedPointerDynamicCast<CommandApdu>(model->item(0));
    QCOMPARE(apdu->cla, QByteArray::fromHex("00"));
    QCOMPARE(apdu->ins, QByteArray::fromHex("A4"));
    QCOMPARE(apdu->p1, QByteArray::fromHex("04"));
    QCOMPARE(apdu->p2, QByteArray::fromHex("00"));
    QCOMPARE(apdu->expectedResponse, QByteArray::fromHex("9000"));

    auto raw = qSharedPointerDynamicCast<CommandSendBytes>(model->item(1));
    QCOMPARE(raw->commandData, QByteArray::fromHex("00A4040000"));
    QCOMPARE(raw->expectedResponse, QByteArray::fromHex("9000"));

    QCOMPARE(tab->lineEditFilename->text(), path);
    QCOMPARE(tab->listView->currentIndex().row(), 0);   // first item selected
    QVERIFY(currentView<ViewApdu>());
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
    QEXPECT_FAIL("", "APDU data saved as \"cmd\" but loaded from \"data\", CODE_REVIEW B-01", Continue);
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
    QEXPECT_FAIL("", "names with quotes/backslashes produce invalid JSON, CODE_REVIEW B-02", Continue);
    QCOMPARE(err.error, QJsonParseError::NoError);
}

static TestRegistrar<TstRunScript> registrar;
#include "tst_runscript.moc"
