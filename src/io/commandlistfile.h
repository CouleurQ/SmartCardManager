#ifndef COMMANDLISTFILE_H
#define COMMANDLISTFILE_H

#include <QByteArray>
#include <QList>
#include <QString>


// Command list file ("Send command" tab): { version, dateTime, id, apduList: [ {name, cmd} ] }
// Schema: res/commandListScheme.json
class CommandListFile
{
public:
    static constexpr const char *Version = "1.0";

    struct Entry {
        QString name;
        QByteArray data;
    };

    // On failure false is returned, *error describes the problem and *entries is left untouched.
    static bool load(const QString &filePath, QList<Entry> *entries, QString *error);
    static bool save(const QString &filePath, const QList<Entry> &entries, QString *error);
};

#endif
