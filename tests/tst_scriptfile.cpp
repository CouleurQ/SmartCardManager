#include "testregistry.h"
#include "io/scriptfile.h"
#include "io/commandlistfile.h"
#include "commands/commandfactory.h"
#include "commands/commandsendbytes.h"
#include "commands/commandapdu.h"
#include "commands/commandterminal.h"

#include <QTest>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonObject>

class TstScriptFile : public QObject
{
    Q_OBJECT

private slots:
    void factoryCreatesKnownTypes();
    void cloneIsIndependentCopy();
    void commandJsonRoundTrip();
    void scriptRoundTrip();
    void scriptKeepsFieldOrder();
    void scriptWithSpecialCharacters();
    void scriptRejectsWrongVersion();
    void scriptRejectsGarbage();
    void scriptMissingFileReportsError();
    void scriptSkipsUnknownTypes();
    void failedLoadLeavesResultUntouched();
    void commandListRoundTrip();
    void commandListRejectsWrongVersion();
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

static QJsonObject toObject(const Command::JsonFields &fields)
{
    QJsonObject object;
    for (const auto &field : fields)
        object.insert(field.first, field.second);
    return object;
}

void TstScriptFile::factoryCreatesKnownTypes()
{
    QCOMPARE(CommandFactory::typeNames(), QStringList({"SendBytes", "SendAPDU", "TerminalCmd"}));
    for (const QString &type : CommandFactory::typeNames())
        QCOMPARE(CommandFactory::create(type)->typeName(), type);
    QVERIFY(CommandFactory::create("Nope").isNull());
}

void TstScriptFile::cloneIsIndependentCopy()
{
    CommandApdu apdu;
    apdu.commandName = "x";
    apdu.cla = QByteArray::fromHex("80");

    QSharedPointer<Command> copy = apdu.clone();
    auto typed = qSharedPointerCast<CommandApdu>(copy);
    QCOMPARE(typed->commandName, QString("x"));
    QCOMPARE(typed->cla, QByteArray::fromHex("80"));

    typed->commandName = "changed";
    QCOMPARE(apdu.commandName, QString("x"));
}

void TstScriptFile::commandJsonRoundTrip()
{
    CommandApdu apdu;
    apdu.cla = QByteArray::fromHex("80");
    apdu.ins = QByteArray::fromHex("CA");
    apdu.p1 = QByteArray::fromHex("9F");
    apdu.p2 = QByteArray::fromHex("7F");
    apdu.commandData = QByteArray::fromHex("0102");
    apdu.le = QByteArray::fromHex("00");
    apdu.expectedResponse = QByteArray::fromHex("9000");

    CommandApdu back;
    back.fromJson(toObject(apdu.toJson()));
    QCOMPARE(back.cla, apdu.cla);
    QCOMPARE(back.ins, apdu.ins);
    QCOMPARE(back.p1, apdu.p1);
    QCOMPARE(back.p2, apdu.p2);
    QCOMPARE(back.commandData, apdu.commandData);
    QCOMPARE(back.le, apdu.le);
    QCOMPARE(back.expectedResponse, apdu.expectedResponse);

    CommandTerminal term;
    term.action = CommandTerminal::ActionType::ColdReset;
    CommandTerminal termBack;
    termBack.fromJson(toObject(term.toJson()));
    QCOMPARE(termBack.action, CommandTerminal::ActionType::ColdReset);

    // no action is not confused with the first real action
    CommandTerminal none;
    CommandTerminal noneBack;
    noneBack.action = CommandTerminal::ActionType::WarmReset;
    noneBack.fromJson(toObject(none.toJson()));
    QCOMPARE(noneBack.action, CommandTerminal::ActionType::None);
}

void TstScriptFile::scriptRoundTrip()
{
    QTemporaryDir dir;
    auto bytes = QSharedPointer<CommandSendBytes>::create();
    bytes->commandName = "bytes";
    bytes->commandData = QByteArray::fromHex("00A40400");
    auto apdu = QSharedPointer<CommandApdu>::create();
    apdu->commandData = QByteArray::fromHex("A000");
    auto term = QSharedPointer<CommandTerminal>::create();
    term->action = CommandTerminal::ActionType::Connect;

    const QString path = dir.filePath("s.json");
    QString error;
    QVERIFY2(ScriptFile::save(path, {bytes, apdu, term}, &error), qPrintable(error));

    QList<QSharedPointer<Command>> loaded;
    QVERIFY2(ScriptFile::load(path, &loaded, &error), qPrintable(error));
    QCOMPARE(loaded.size(), 3);
    QCOMPARE(loaded[0]->typeName(), QString("SendBytes"));
    QCOMPARE(qSharedPointerCast<CommandSendBytes>(loaded[0])->commandData, bytes->commandData);
    QCOMPARE(qSharedPointerCast<CommandApdu>(loaded[1])->commandData, apdu->commandData);
    QCOMPARE(qSharedPointerCast<CommandTerminal>(loaded[2])->action, CommandTerminal::ActionType::Connect);
}

void TstScriptFile::scriptKeepsFieldOrder()
{
    QTemporaryDir dir;
    auto bytes = QSharedPointer<CommandSendBytes>::create();
    const QString path = dir.filePath("s.json");
    QString error;
    QVERIFY(ScriptFile::save(path, {bytes}, &error));

    QFile f(path);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QString text = QString::fromUtf8(f.readAll());
    QVERIFY(text.indexOf("\"name\"") < text.indexOf("\"seq\""));
    QVERIFY(text.indexOf("\"seq\"") < text.indexOf("\"type\""));
    QVERIFY(text.indexOf("\"type\"") < text.indexOf("\"cmd\""));
    QVERIFY(QJsonDocument::fromJson(text.toUtf8()).isObject());
}

void TstScriptFile::scriptWithSpecialCharacters()
{
    QTemporaryDir dir;
    auto bytes = QSharedPointer<CommandSendBytes>::create();
    bytes->commandName = "say \"hi\" \\ back\tslash\nline";
    const QString path = dir.filePath("s.json");
    QString error;
    QVERIFY(ScriptFile::save(path, {bytes}, &error));

    QList<QSharedPointer<Command>> loaded;
    QVERIFY2(ScriptFile::load(path, &loaded, &error), qPrintable(error));
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded[0]->commandName, bytes->commandName);
}

void TstScriptFile::scriptRejectsWrongVersion()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "v.json", R"({"version": "9.9", "items": []})");
    QList<QSharedPointer<Command>> loaded;
    QString error;
    QVERIFY(!ScriptFile::load(path, &loaded, &error));
    QVERIFY(error.contains("version"));
}

void TstScriptFile::scriptRejectsGarbage()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "g.json", "{not json");
    QList<QSharedPointer<Command>> loaded;
    QString error;
    QVERIFY(!ScriptFile::load(path, &loaded, &error));
    QVERIFY(error.contains("parse"));
}

void TstScriptFile::scriptMissingFileReportsError()
{
    QList<QSharedPointer<Command>> loaded;
    QString error;
    QVERIFY(!ScriptFile::load("/nonexistent/dir/x.json", &loaded, &error));
    QVERIFY(!error.isEmpty());
}

void TstScriptFile::scriptSkipsUnknownTypes()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "u.json", R"({"version": "1.0", "items": [
        {"name": "a", "type": "Mystery"}, {"name": "b", "type": "TerminalCmd", "action": "Connect"}]})");
    QList<QSharedPointer<Command>> loaded;
    QString error;
    QVERIFY(ScriptFile::load(path, &loaded, &error));
    QCOMPARE(loaded.size(), 1);
    QCOMPARE(loaded[0]->commandName, QString("b"));
}

void TstScriptFile::failedLoadLeavesResultUntouched()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "bad.json", "nope");
    QList<QSharedPointer<Command>> loaded{QSharedPointer<CommandSendBytes>::create()};
    QString error;
    QVERIFY(!ScriptFile::load(path, &loaded, &error));
    QCOMPARE(loaded.size(), 1);
}

void TstScriptFile::commandListRoundTrip()
{
    QTemporaryDir dir;
    const QString path = dir.filePath("c.json");
    QString error;
    QVERIFY(CommandListFile::save(path, {{"Select \"ISD\"", QByteArray::fromHex("00A40400")}, {"Empty", {}}}, &error));

    QList<CommandListFile::Entry> entries;
    QVERIFY2(CommandListFile::load(path, &entries, &error), qPrintable(error));
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries[0].name, QString("Select \"ISD\""));
    QCOMPARE(entries[0].data, QByteArray::fromHex("00A40400"));
    QVERIFY(entries[1].data.isEmpty());
}

void TstScriptFile::commandListRejectsWrongVersion()
{
    QTemporaryDir dir;
    const QString path = writeFile(dir, "v.json", R"({"version": "2.0", "apduList": []})");
    QList<CommandListFile::Entry> entries;
    QString error;
    QVERIFY(!CommandListFile::load(path, &entries, &error));
    QVERIFY(error.contains("version"));
}

static TestRegistrar<TstScriptFile> registrar;
#include "tst_scriptfile.moc"
