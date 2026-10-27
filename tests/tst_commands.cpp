#include "testregistry.h"
#include "commands/command.h"
#include "commands/commandapdu.h"
#include "commands/commandsendbytes.h"
#include "commands/commandterminal.h"
#include "commands/runscriptmodel.h"

#include <QTest>
#include <QSignalSpy>
#include <QAbstractItemModelTester>

namespace {
QSharedPointer<Command> makeSendBytes(const QString &name)
{
    auto cmd = QSharedPointer<CommandSendBytes>::create();
    cmd->commandName = name;
    return cmd;
}

QStringList names(const RunScriptModel &model)
{
    QStringList list;
    for (int i = 0; i < model.rowCount(); ++i)
        list << model.index(i, 0).data().toString();
    return list;
}
}

class TstCommands : public QObject
{
    Q_OBJECT

private slots:
    // --- command classes ---
    void defaults();
    void typeDiscriminator();
    void copyConstructors();
    void terminalActionNoneIsDistinctFromWarmReset();
    void terminalActionStrings();

    // --- RunScriptModel ---
    void modelPassesQtModelTester();
    void modelAddAndAccess();
    void modelOutOfRangeAccess();
    void modelDataRoles();
    void modelSetData();
    void modelRemoveAndTake();
    void modelInsertRowSingle();
    void modelInsertRowKeepsOrderOfMultipleCommands();
    void modelClear();
    void modelCommandType();
};

void TstCommands::defaults()
{
    QCOMPARE(CommandSendBytes().commandName, QString("New Send Bytes"));
    QCOMPARE(CommandApdu().commandName, QString("New APDU command"));
    QCOMPARE(CommandTerminal().commandName, QString("New Terminal Command"));
    QVERIFY(CommandSendBytes().commandData.isEmpty());
    QVERIFY(CommandSendBytes().expectedResponse.isEmpty());
}

void TstCommands::typeDiscriminator()
{
    QCOMPARE(CommandSendBytes().typeName(), QString("SendBytes"));
    QCOMPARE(CommandApdu().typeName(), QString("SendAPDU"));
    QCOMPARE(CommandTerminal().typeName(), QString("TerminalCmd"));
}

void TstCommands::copyConstructors()
{
    CommandApdu apdu;
    apdu.commandName = "select";
    apdu.cla = QByteArray::fromHex("00");
    apdu.ins = QByteArray::fromHex("A4");
    apdu.p1 = QByteArray::fromHex("04");
    apdu.p2 = QByteArray::fromHex("00");
    apdu.commandData = QByteArray::fromHex("A000");
    apdu.le = QByteArray::fromHex("00");
    apdu.expectedResponse = QByteArray::fromHex("9000");

    CommandApdu apduCopy(apdu);
    QCOMPARE(apduCopy.commandName, apdu.commandName);
    QCOMPARE(apduCopy.cla, apdu.cla);
    QCOMPARE(apduCopy.ins, apdu.ins);
    QCOMPARE(apduCopy.p1, apdu.p1);
    QCOMPARE(apduCopy.p2, apdu.p2);
    QCOMPARE(apduCopy.commandData, apdu.commandData);
    QCOMPARE(apduCopy.le, apdu.le);
    QCOMPARE(apduCopy.expectedResponse, apdu.expectedResponse);

    CommandSendBytes bytes;
    bytes.commandName = "raw";
    bytes.commandData = QByteArray::fromHex("80CA9F7F00");
    bytes.expectedResponse = QByteArray::fromHex("9000");
    CommandSendBytes bytesCopy(bytes);
    QCOMPARE(bytesCopy.commandName, bytes.commandName);
    QCOMPARE(bytesCopy.commandData, bytes.commandData);
    QCOMPARE(bytesCopy.expectedResponse, bytes.expectedResponse);

    CommandTerminal term;
    term.commandName = "reset";
    term.action = CommandTerminal::ActionType::ColdReset;
    CommandTerminal termCopy(term);
    QCOMPARE(termCopy.commandName, term.commandName);
    QCOMPARE(termCopy.action, CommandTerminal::ActionType::ColdReset);

    // copies are independent
    apduCopy.commandName = "changed";
    QCOMPARE(apdu.commandName, QString("select"));
}

// BUG: enum ActionType { None, WarmReset = 0, ... } gives None and WarmReset the same value.
void TstCommands::terminalActionNoneIsDistinctFromWarmReset()
{
    QVERIFY(CommandTerminal::ActionType::None != CommandTerminal::ActionType::WarmReset);

    QCOMPARE(CommandTerminal::getActionString(CommandTerminal::ActionType::None), QString());
}

void TstCommands::terminalActionStrings()
{
    using A = CommandTerminal::ActionType;
    QCOMPARE(CommandTerminal::getActionString(A::WarmReset), QString("WarmReset"));
    QCOMPARE(CommandTerminal::getActionString(A::ColdReset), QString("ColdReset"));
    QCOMPARE(CommandTerminal::getActionString(A::WaitForCard), QString("WaitForCard"));
    QCOMPARE(CommandTerminal::getActionString(A::WaitForRemoval), QString("WaitForRemoval"));
    QCOMPARE(CommandTerminal::getActionString(A::Connect), QString("Connect"));
    QCOMPARE(CommandTerminal::getActionString(A::Disconnect), QString("Disconnect"));
    QCOMPARE(CommandTerminal::getActionString(A::_count_), QString());
}

void TstCommands::modelPassesQtModelTester()
{
    RunScriptModel model;
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);

    model.addCommand(makeSendBytes("a"));
    model.addCommand(makeSendBytes("b"));
    model.insertCommands(1, {makeSendBytes("c")});
    model.takeCommands(0);
    model.removeCommand(0);
    model.clear();
}

void TstCommands::modelAddAndAccess()
{
    RunScriptModel model;
    QSignalSpy inserted(&model, &QAbstractItemModel::rowsInserted);

    model.addCommand(makeSendBytes("first"));
    model.addCommand(makeSendBytes("second"));

    QCOMPARE(model.rowCount(), 2);
    QCOMPARE(inserted.count(), 2);
    QCOMPARE(model.item(0)->commandName, QString("first"));
    QCOMPARE(model.itemFromIndex(model.index(1, 0))->commandName, QString("second"));
}

void TstCommands::modelOutOfRangeAccess()
{
    RunScriptModel model;
    model.addCommand(makeSendBytes("only"));

    QVERIFY(model.item(-1).isNull());
    QVERIFY(model.item(1).isNull());
    QVERIFY(model.itemFromIndex(QModelIndex()).isNull());
    QVERIFY(model.takeCommands(5).isEmpty());
    QVERIFY(!model.removeCommand(5));
    QVERIFY(!model.removeCommand(-1));
    QCOMPARE(model.commandType(7), QString());

    // insertRow outside [0, rowCount] is ignored
    model.insertCommands(-1, {makeSendBytes("x")});
    model.insertCommands(5, {makeSendBytes("x")});
    QCOMPARE(model.rowCount(), 1);
}

void TstCommands::modelDataRoles()
{
    RunScriptModel model;
    auto cmd = makeSendBytes("named");
    model.addCommand(cmd);

    QCOMPARE(model.data(model.index(0, 0), Qt::DisplayRole).toString(), QString("named"));
    QCOMPARE(model.data(model.index(0, 0), Qt::UserRole).value<QSharedPointer<Command>>(), cmd);
    QVERIFY(!model.data(model.index(0, 0), Qt::DecorationRole).isValid());
    QVERIFY(!model.data(QModelIndex(), Qt::DisplayRole).isValid());
}

void TstCommands::modelSetData()
{
    RunScriptModel model;
    model.addCommand(makeSendBytes("old"));
    QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);

    auto replacement = makeSendBytes("new");
    QVERIFY(model.setData(model.index(0, 0), QVariant::fromValue(replacement), Qt::UserRole));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(model.data(model.index(0, 0)).toString(), QString("new"));

    // wrong role / invalid index / wrong payload are rejected
    QVERIFY(!model.setData(model.index(0, 0), "text", Qt::DisplayRole));
    QVERIFY(!model.setData(QModelIndex(), QVariant::fromValue(replacement), Qt::UserRole));
    QVERIFY(!model.setData(model.index(0, 0), 42, Qt::UserRole));
    QCOMPARE(changed.count(), 1);
}

void TstCommands::modelRemoveAndTake()
{
    RunScriptModel model;
    for (const char *n : {"a", "b", "c", "d"})
        model.addCommand(makeSendBytes(n));

    QSignalSpy removed(&model, &QAbstractItemModel::rowsRemoved);

    QVERIFY(model.removeCommand(1));
    QCOMPARE(names(model), QStringList({"a", "c", "d"}));

    auto taken = model.takeCommands(0);
    QCOMPARE(taken.size(), 1);
    QCOMPARE(taken.first()->commandName, QString("a"));
    QCOMPARE(names(model), QStringList({"c", "d"}));
    QCOMPARE(removed.count(), 2);
}

void TstCommands::modelInsertRowSingle()
{
    RunScriptModel model;
    model.addCommand(makeSendBytes("a"));
    model.addCommand(makeSendBytes("b"));

    model.insertCommands(0, {makeSendBytes("front")});
    model.insertCommands(3, {makeSendBytes("back")});
    model.insertCommands(2, {makeSendBytes("mid")});

    QCOMPARE(names(model), QStringList({"front", "a", "mid", "b", "back"}));
}

// BUG: insertRow() inserts every command at the same index, reversing the batch.
void TstCommands::modelInsertRowKeepsOrderOfMultipleCommands()
{
    RunScriptModel model;
    model.addCommand(makeSendBytes("a"));

    model.insertCommands(0, {makeSendBytes("x"), makeSendBytes("y")});

    QCOMPARE(model.rowCount(), 3);
    QCOMPARE(names(model), QStringList({"x", "y", "a"}));
}

void TstCommands::modelClear()
{
    RunScriptModel model;
    model.addCommand(makeSendBytes("a"));
    QSignalSpy reset(&model, &QAbstractItemModel::modelReset);

    model.clear();

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(reset.count(), 1);
}

void TstCommands::modelCommandType()
{
    RunScriptModel model;
    model.addCommand(QSharedPointer<CommandSendBytes>::create());
    model.addCommand(QSharedPointer<CommandApdu>::create());
    model.addCommand(QSharedPointer<CommandTerminal>::create());

    QCOMPARE(model.commandType(0), QString("SendBytes"));
    QCOMPARE(model.commandType(1), QString("SendAPDU"));
    QCOMPARE(model.commandType(2), QString("TerminalCmd"));
}

static TestRegistrar<TstCommands> registrar;
#include "tst_commands.moc"
