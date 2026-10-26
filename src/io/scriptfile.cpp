#include "scriptfile.h"

#include "io/jsonwriter.h"
#include "commands/commandfactory.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QUuid>


bool ScriptFile::load(const QString &filePath, QList<QSharedPointer<Command>> *commands, QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QString("Failed to open file: %1").arg(file.errorString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull()) {
        *error = QString("Failed to parse script file: %1").arg(parseError.errorString());
        return false;
    }

    const QJsonObject root = doc.object();
    const QString version = root["version"].toString();
    if (version != Version) {
        *error = QString("Invalid script file version: %1").arg(version);
        return false;
    }

    QList<QSharedPointer<Command>> result;
    const QJsonArray items = root["items"].toArray();
    for (const QJsonValue &value : items) {
        const QJsonObject item = value.toObject();
        QSharedPointer<Command> command = CommandFactory::create(item["type"].toString());
        if (!command)
            continue;   // unknown item types are skipped

        command->commandName = item["name"].toString();
        command->fromJson(item);
        result.append(command);
    }

    *commands = result;
    return true;
}


bool ScriptFile::save(const QString &filePath, const QList<QSharedPointer<Command>> &commands, QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *error = QString("Cannot write to file: %1").arg(file.errorString());
        return false;
    }

    // written by hand to keep the key order for human editing
    QTextStream out(&file);
    out << "{\n";
    out << "  \"version\": " << JsonWriter::value(Version) << ",\n";
    out << "  \"dateTime\": " << JsonWriter::value(QDateTime::currentDateTime().toString(Qt::ISODate)) << ",\n";
    out << "  \"id\": " << JsonWriter::value(QUuid::createUuid().toString(QUuid::WithoutBraces)) << ",\n";
    out << "  \"items\": [\n";

    for (int row = 0; row < commands.size(); ++row) {
        const QSharedPointer<Command> &command = commands[row];

        out << "    {\n";
        out << "      \"name\": " << JsonWriter::value(command->commandName) << ",\n";
        out << "      \"seq\": " << row << ",\n";
        out << "      \"type\": " << JsonWriter::value(command->typeName());

        for (const auto &field : command->toJson())
            out << ",\n      " << JsonWriter::value(field.first) << ": " << JsonWriter::value(field.second);

        out << "\n    }" << (row + 1 < commands.size() ? "," : "") << "\n";
    }

    out << "  ]\n";
    out << "}\n";
    out.flush();

    if (file.error() != QFileDevice::NoError) {
        *error = QString("Cannot write to file: %1").arg(file.errorString());
        return false;
    }
    return true;
}
