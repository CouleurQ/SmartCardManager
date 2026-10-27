#include "testregistry.h"
#include "mocktransport.h"
#include "testutil.h"
#include "privateaccess.h"
#include "ui_mainwindow.h"
#include "ui/listreorderhelper.h"

#include <QTest>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTemporaryDir>

class TstSendCommand : public QObject
{
    Q_OBJECT

private:
    MockTransport *transport = nullptr;
    CardSession *session = nullptr;
    QMainWindow *window = nullptr;
    Ui::MainWindow *ui = nullptr;
    TabHandlerSendCommand *tab = nullptr;
    QStandardItemModel *model = nullptr;

    QStringList names() const {
        QStringList list;
        for (int i = 0; i < model->rowCount(); ++i)
            list << model->item(i)->text();
        return list;
    }
    void select(int row) { tab->listView->setCurrentIndex(model->index(row, 0)); }

private slots:
    void init();
    void cleanup();

    void addCreatesSelectedItem();
    void editingNameUpdatesItem();
    void clearingNameUpdatesItem();
    void editingHexUpdatesItemData();
    void clearingHexUpdatesItemData();
    void selectingItemShowsItsContent();
    void duplicateAppendsCopy();
    void moveUpAndDown();
    void deleteRemovesCurrent();
    void clearRemovesAll();

    void sendWithoutCardLogsCommandAndNotifies();
    void sendWithCardLogsResponse();
    void sendEmptyCommandDoesNothing();
    void responseShowsStatusWordAndFullResponse();
    void addButtonAddsCommand();
    void switchingCommandClearsResponse();
    void lateResponseIsNotShownForAnotherCommand();
    void outputFieldsAreGrey();
    void dropMovesItemsWithoutLosingThem();
    void doubleClickSends();

    void loadReadsCommandList();
    void loadRejectsWrongVersion();
    void loadReportsMissingFile();
    void loadReportsInvalidJson();
    void loadReplacesExistingItems();
    void dropOfJsonFileLoadsIt();
    void saveAndLoadRoundTrip();
    void saveAddsJsonExtension();
};

static const char *kTwoCommands = R"({
  "version": "1.0", "dateTime": "2025-04-23T10:30:00+02:00",
  "id": "01234567-89ab-cdef-0123-456789abcdef",
  "apduList": [ {"name": "Select ISD", "cmd": "00 A4 04 00"},
                {"name": "Get CPLC Data", "cmd": "80 CA 9F 7F 00"} ] })";

static QString writeFile(const QTemporaryDir &dir, const QString &name, const QByteArray &content)
{
    const QString path = dir.filePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        qFatal("cannot write %s", qPrintable(path));
    f.write(content);
    return path;
}

void TstSendCommand::init()
{
    window = new QMainWindow;
    ui = new Ui::MainWindow;
    ui->setupUi(window);
    transport = new MockTransport;
    session = new CardSession(transport);
    tab = new TabHandlerSendCommand(ui->tabSendCommand, session);
    model = qobject_cast<QStandardItemModel *>(tab->listView->model());
    window->show();
}

void TstSendCommand::cleanup()
{
    delete tab;
    delete window;   // owns the widgets referenced by ui
    delete ui;
    delete session;
    delete transport;
    session = nullptr; transport = nullptr;
    tab = nullptr; window = nullptr; ui = nullptr; model = nullptr;
}

void TstSendCommand::addCreatesSelectedItem()
{
    tab->buttonActionAdd();

    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(names(), QStringList({"new command"}));
    QCOMPARE(tab->listView->currentIndex().row(), 0);
    QCOMPARE(tab->lineEditCommandName->text(), QString("new command"));
}

void TstSendCommand::editingNameUpdatesItem()
{
    tab->buttonActionAdd();
    tab->lineEditCommandName->setText("Select ISD");
    QCOMPARE(names(), QStringList({"Select ISD"}));
}

// BUG: updateCommandName() returns early for empty text, so the list keeps the old name.
void TstSendCommand::clearingNameUpdatesItem()
{
    tab->buttonActionAdd();
    tab->lineEditCommandName->setText("abc");
    tab->lineEditCommandName->clear();

    QCOMPARE(names(), QStringList({""}));
}

void TstSendCommand::editingHexUpdatesItemData()
{
    tab->buttonActionAdd();
    tab->hexEditCommandApdu->setData(QByteArray::fromHex("00A40400"));
    QCOMPARE(model->item(0)->data(Qt::UserRole).toByteArray(), QByteArray::fromHex("00A40400"));
}

// BUG: same early return in updateCommandApdu() - emptied data keeps the previous bytes.
void TstSendCommand::clearingHexUpdatesItemData()
{
    tab->buttonActionAdd();
    tab->hexEditCommandApdu->setData(QByteArray::fromHex("00A40400"));
    tab->hexEditCommandApdu->setData(QByteArray());

    QVERIFY(model->item(0)->data(Qt::UserRole).toByteArray().isEmpty());
}

void TstSendCommand::selectingItemShowsItsContent()
{
    tab->_buttonActionAdd("one", QByteArray::fromHex("0102"));
    tab->_buttonActionAdd("two", QByteArray::fromHex("0304"));

    select(0);
    QCOMPARE(tab->lineEditCommandName->text(), QString("one"));
    QCOMPARE(tab->hexEditCommandApdu->toPlainText(), QString("01 02"));

    select(1);
    QCOMPARE(tab->lineEditCommandName->text(), QString("two"));
    QCOMPARE(tab->hexEditCommandApdu->toPlainText(), QString("03 04"));
    // selecting must not modify the stored data
    QCOMPARE(model->item(0)->data(Qt::UserRole).toByteArray(), QByteArray::fromHex("0102"));
}

void TstSendCommand::duplicateAppendsCopy()
{
    tab->_buttonActionAdd("one", QByteArray::fromHex("0102"));
    tab->buttonActionDuplicate();

    QCOMPARE(names(), QStringList({"one", "one_copy"}));
    QCOMPARE(model->item(1)->data(Qt::UserRole).toByteArray(), QByteArray::fromHex("0102"));
}

void TstSendCommand::moveUpAndDown()
{
    for (const char *n : {"a", "b", "c"})
        tab->_buttonActionAdd(n, QByteArray());

    select(2);
    tab->buttonActionUp();
    QCOMPARE(names(), QStringList({"a", "c", "b"}));
    QCOMPARE(tab->listView->currentIndex().row(), 1);

    tab->buttonActionDown();
    QCOMPARE(names(), QStringList({"a", "b", "c"}));

    select(0);
    tab->buttonActionUp();               // already first: no-op
    QCOMPARE(names(), QStringList({"a", "b", "c"}));
    select(2);
    tab->buttonActionDown();             // already last: no-op
    QCOMPARE(names(), QStringList({"a", "b", "c"}));
}

void TstSendCommand::deleteRemovesCurrent()
{
    for (const char *n : {"a", "b", "c"})
        tab->_buttonActionAdd(n, QByteArray());
    select(1);
    tab->buttonActionDelete();
    QCOMPARE(names(), QStringList({"a", "c"}));
}

void TstSendCommand::clearRemovesAll()
{
    tab->_buttonActionAdd("a", QByteArray());
    tab->buttonActionClear();
    QCOMPARE(model->rowCount(), 0);
}

void TstSendCommand::sendWithoutCardLogsCommandAndNotifies()
{
    QSignalSpy bytes(tab, &TabHandlerSendCommand::logBytes);
    QSignalSpy updates(tab, &TabHandlerSendCommand::update);
    tab->hexEditCommandApdu->setData(QByteArray::fromHex("00A40400"));

    tab->buttonActionSend();

    QTRY_COMPARE(updates.count(), 1);                             // the answer arrives asynchronously
    QCOMPARE(bytes.count(), 1);                                   // only the command, no response
    QCOMPARE(bytes.first().at(0).value<LogType>(), LogType::CardCmd);
    QCOMPARE(bytes.first().at(1).toByteArray(), QByteArray::fromHex("00A40400"));
}

void TstSendCommand::sendWithCardLogsResponse()
{
    transport->answers.insert(QByteArray::fromHex("00A40400"), QByteArray::fromHex("6A82"));
    session->setReader("Mock Reader 0");
    session->connectCard();
    QSignalSpy bytes(tab, &TabHandlerSendCommand::logBytes);
    QSignalSpy updates(tab, &TabHandlerSendCommand::update);
    tab->hexEditCommandApdu->setData(QByteArray::fromHex("00A40400"));

    tab->buttonActionSend();

    QTRY_COMPARE(updates.count(), 1);
    QCOMPARE(bytes.count(), 2);
    QCOMPARE(bytes.at(1).at(0).value<LogType>(), LogType::CardRsp);
    QCOMPARE(bytes.at(1).at(1).toByteArray(), QByteArray::fromHex("6A82"));
}

void TstSendCommand::responseShowsStatusWordAndFullResponse()
{
    transport->answers.insert(QByteArray::fromHex("80CA9F7F00"), QByteArray::fromHex("AABB9000"));
    transport->answers.insert(QByteArray::fromHex("00A40400"), QByteArray::fromHex("6A82"));
    session->setReader("Mock Reader 0");
    session->connectCard();
    QSignalSpy updates(tab, &TabHandlerSendCommand::update);

    tab->hexEditCommandApdu->setData(QByteArray::fromHex("80CA9F7F00"));
    tab->buttonActionSend();
    QTRY_COMPARE(updates.count(), 1);
    QCOMPARE(tab->lineEditSw->text(), QString("90 00"));
    QCOMPARE(tab->lineEditSwText->text(), QString("Success"));
    QCOMPARE(tab->textEditResponse->toPlainText(), QString("AA BB 90 00"));

    tab->hexEditCommandApdu->setData(QByteArray::fromHex("00A40400"));
    tab->buttonActionSend();
    QTRY_COMPARE(updates.count(), 2);
    QCOMPARE(tab->lineEditSw->text(), QString("6A 82"));
    QCOMPARE(tab->lineEditSwText->text(), QString("File or application not found"));
    QCOMPARE(tab->textEditResponse->toPlainText(), QString("6A 82"));
}

void TstSendCommand::switchingCommandClearsResponse()
{
    transport->answers.insert(QByteArray::fromHex("80CA9F7F00"), QByteArray::fromHex("AABB9000"));
    session->setReader("Mock Reader 0");
    session->connectCard();
    tab->_buttonActionAdd("a", QByteArray::fromHex("80CA9F7F00"));
    tab->_buttonActionAdd("b", QByteArray::fromHex("00A40400"));
    select(0);
    QSignalSpy updates(tab, &TabHandlerSendCommand::update);
    tab->buttonActionSend();
    QTRY_COMPARE(updates.count(), 1);
    QCOMPARE(tab->lineEditSw->text(), QString("90 00"));

    select(1);
    QVERIFY(tab->lineEditSw->text().isEmpty());
    QVERIFY(tab->lineEditSwText->text().isEmpty());
    QVERIFY(tab->textEditResponse->toPlainText().isEmpty());
}

void TstSendCommand::lateResponseIsNotShownForAnotherCommand()
{
    session->setReader("Mock Reader 0");
    session->connectCard();
    tab->_buttonActionAdd("a", QByteArray::fromHex("80CA9F7F00"));
    tab->_buttonActionAdd("b", QByteArray::fromHex("00A40400"));
    select(0);
    tab->buttonActionSend();
    select(1);
    tab->onResponseReceived(QByteArray(), QByteArray::fromHex("9000"));
    QVERIFY(tab->lineEditSw->text().isEmpty());
}

void TstSendCommand::outputFieldsAreGrey()
{
    for (QWidget *w : {static_cast<QWidget *>(tab->lineEditSw), static_cast<QWidget *>(tab->lineEditSwText), static_cast<QWidget *>(tab->textEditResponse)})
        QVERIFY(w->palette().color(QPalette::Base) != tab->lineEditCommandName->palette().color(QPalette::Base));
}

void TstSendCommand::dropMovesItemsWithoutLosingThem()
{
    tab->_buttonActionAdd("a", QByteArray::fromHex("01"));
    tab->_buttonActionAdd("b", QByteArray::fromHex("02"));
    tab->_buttonActionAdd("c", QByteArray::fromHex("03"));
    window->resize(900, 700);
    ui->tabWidget->setCurrentWidget(ui->tabSendCommand);
    QVERIFY(QTest::qWaitForWindowExposed(window));
    auto belowRow = [&](int row) { return tab->listView->visualRect(model->index(row, 0)).bottomLeft() + QPoint(5, -2); };

    select(0);
    QVERIFY(tab->reorder->drop(belowRow(2)));
    QCOMPARE(names(), QStringList({"b", "c", "a"}));
    QCOMPARE(model->item(2)->data(Qt::UserRole).toByteArray(), QByteArray::fromHex("01"));
    QCOMPARE(tab->listView->currentIndex().row(), 2);

    // the same through real drag events (the mime type is the helper's own)
    QMimeData mime;
    mime.setData("application/x-smartcardmanager-listrows", "rows");
    select(2);
    QDragEnterEvent enter(belowRow(0), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(tab->listView->viewport(), &enter);
    QVERIFY(enter.isAccepted());
    QDropEvent drop(QPointF(tab->listView->visualRect(model->index(0, 0)).topLeft() + QPoint(5, 2)), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(tab->listView->viewport(), &drop);
    QCOMPARE(names(), QStringList({"a", "b", "c"}));
    QCOMPARE(drop.dropAction(), Qt::CopyAction);   // not a move: the view must not delete the source row

    select(2);
    const QPoint aboveFirst = tab->listView->visualRect(model->index(0, 0)).topLeft() + QPoint(5, 2);
    QVERIFY(tab->reorder->drop(aboveFirst));
    QCOMPARE(names(), QStringList({"c", "a", "b"}));
    select(0);
    QVERIFY(!tab->reorder->drop(aboveFirst));      // already first
}

void TstSendCommand::addButtonAddsCommand()
{
    QCOMPARE(model->rowCount(), 0);
    tab->buttonAdd->click();
    QCOMPARE(model->rowCount(), 1);
    QCOMPARE(tab->listView->currentIndex().row(), 0);
}

void TstSendCommand::sendEmptyCommandDoesNothing()
{
    QSignalSpy bytes(tab, &TabHandlerSendCommand::logBytes);
    QSignalSpy updates(tab, &TabHandlerSendCommand::update);
    tab->buttonActionSend();
    QCOMPARE(bytes.count(), 0);
    QCOMPARE(updates.count(), 0);
}

void TstSendCommand::doubleClickSends()
{
    tab->_buttonActionAdd("one", QByteArray::fromHex("0102"));
    QSignalSpy bytes(tab, &TabHandlerSendCommand::logBytes);

    tab->onItemDoubleClicked(model->index(0, 0));

    QCOMPARE(bytes.count(), 1);
    QCOMPARE(bytes.first().at(1).toByteArray(), QByteArray::fromHex("0102"));
}

void TstSendCommand::loadReadsCommandList()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "cmds.json", kTwoCommands);

    tab->_buttonActionLoad(path);

    QCOMPARE(names(), QStringList({"Select ISD", "Get CPLC Data"}));
    QCOMPARE(model->item(0)->data(Qt::UserRole).toByteArray(), QByteArray::fromHex("00A40400"));
    QCOMPARE(model->item(1)->data(Qt::UserRole).toByteArray(), QByteArray::fromHex("80CA9F7F00"));
    QCOMPARE(tab->lineEditFilename->text(), path);
}

void TstSendCommand::loadRejectsWrongVersion()
{
    QTemporaryDir dir;
    QSignalSpy logs(tab, &TabHandlerSendCommand::logString);
    const QString path = writeFile(dir, "v2.json", R"({"version": "2.0", "apduList": []})");

    tab->_buttonActionAdd("keep", QByteArray());
    tab->_buttonActionLoad(path);

    QCOMPARE(logs.count(), 1);
    QCOMPARE(logs.first().at(0).value<LogType>(), LogType::Error);
    QCOMPARE(names(), QStringList({"keep"}));        // existing list untouched
}

void TstSendCommand::loadReportsMissingFile()
{
    QSignalSpy logs(tab, &TabHandlerSendCommand::logString);
    tab->_buttonActionLoad("/nonexistent/dir/file.json");
    QCOMPARE(logs.count(), 1);
    QCOMPARE(logs.first().at(0).value<LogType>(), LogType::Error);
}

// BUG: unparsable JSON is only written to qWarning(); the user gets no feedback in the log view.
void TstSendCommand::loadReportsInvalidJson()
{
    QTemporaryDir dir;
    QSignalSpy logs(tab, &TabHandlerSendCommand::logString);
    const QString path = writeFile(dir, "bad.json", "this is { not json");

    tab->_buttonActionLoad(path);

    QCOMPARE(logs.count(), 1);
}

void TstSendCommand::loadReplacesExistingItems()
{
    QTemporaryDir dir;
    tab->_buttonActionAdd("old", QByteArray());
    tab->_buttonActionLoad(writeFile(dir, "cmds.json", kTwoCommands));
    QCOMPARE(model->rowCount(), 2);
}

void TstSendCommand::dropOfJsonFileLoadsIt()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "cmds.json", kTwoCommands);

    QMimeData mime;
    mime.setUrls({QUrl::fromLocalFile(path)});
    QDragEnterEvent enter(QPoint(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(tab->listView, &enter);
    QVERIFY(enter.isAccepted());

    QDropEvent drop(QPointF(5, 5), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(tab->listView, &drop);

    QCOMPARE(model->rowCount(), 2);
}

void TstSendCommand::saveAndLoadRoundTrip()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("saved.json");

    tab->_buttonActionAdd("Select ISD", QByteArray::fromHex("00A40400"));
    tab->_buttonActionAdd("Get CPLC", QByteArray::fromHex("80CA9F7F00"));

    DialogAutomator dialogs(path);
    tab->buttonActionSave();
    QCOMPARE(dialogs.fileDialogsHandled, 1);
    QVERIFY(QFile::exists(path));

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
    QCOMPARE(root["version"].toString(), QString("1.0"));
    QVERIFY(!root["id"].toString().isEmpty());
    QCOMPARE(root["apduList"].toArray().size(), 2);
    QCOMPARE(root["apduList"].toArray().at(0).toObject()["cmd"].toString(), QString("00 A4 04 00"));

    tab->_buttonActionLoad(path);
    QCOMPARE(names(), QStringList({"Select ISD", "Get CPLC"}));
    QCOMPARE(model->item(1)->data(Qt::UserRole).toByteArray(), QByteArray::fromHex("80CA9F7F00"));
}

void TstSendCommand::saveAddsJsonExtension()
{
    QTemporaryDir dir;
    tab->_buttonActionAdd("x", QByteArray::fromHex("00"));

    DialogAutomator dialogs(dir.filePath("noext"));
    tab->buttonActionSave();

    QVERIFY(QFile::exists(dir.filePath("noext.json")));
}

static TestRegistrar<TstSendCommand> registrar;
#include "tst_sendcommand.moc"
