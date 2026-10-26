#ifndef JSONWRITER_H
#define JSONWRITER_H

#include <QJsonArray>
#include <QJsonDocument>
#include <QString>


namespace JsonWriter {

// JSON text of a single value (quotes, backslashes and control characters escaped). Used to write
// files by hand with a fixed key order, which QJsonObject (sorted keys) cannot do.
inline QString value(const QJsonValue &v)
{
    const QByteArray array = QJsonDocument(QJsonArray{v}).toJson(QJsonDocument::Compact);
    return QString::fromUtf8(array.mid(1, array.size() - 2));
}

}

#endif
