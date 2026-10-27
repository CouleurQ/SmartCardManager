#include "commandlistfile.h"

#include "io/jsonwriter.h"
#include "utility.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QUuid>


bool CommandListFile::load(const QString &filePath, QList<Entry> *entries, QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QString("Failed to open file: %1").arg(file.errorString());
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull()) {
        *error = QString("Failed to parse command file: %1").arg(parseError.errorString());
        return false;
    }

    const QJsonObject root = doc.object();
    const QString version = root["version"].toString();
    if (version != Version) {
        *error = QString("Invalid json version: %1").arg(version);
        return false;
    }

    QList<Entry> result;
    const QJsonArray list = root["apduList"].toArray();
    for (const QJsonValue &value : list) {
        const QJsonObject object = value.toObject();
        result.append({object["name"].toString(), Utility::hexStringToByteArray(object["cmd"].toString())});
    }

    *entries = result;
    return true;
}


bool CommandListFile::save(const QString &filePath, const QList<Entry> &entries, QString *error)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *error = QString("Cannot write to file: %1").arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out << "{\n";
    out << "  \"version\": " << JsonWriter::value(Version) << ",\n";
    out << "  \"dateTime\": " << JsonWriter::value(QDateTime::currentDateTime().toString(Qt::ISODate)) << ",\n";
    out << "  \"id\": " << JsonWriter::value(QUuid::createUuid().toString(QUuid::WithoutBraces)) << ",\n";
    out << "  \"apduList\": [\n";
    for (int i = 0; i < entries.size(); ++i) {
        out << "    {\n";
        out << "      \"name\": " << JsonWriter::value(entries[i].name) << ",\n";
        out << "      \"cmd\": " << JsonWriter::value(Utility::byteArrayToHexString(entries[i].data)) << "\n";
        out << "    }" << (i + 1 < entries.size() ? "," : "") << "\n";
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
