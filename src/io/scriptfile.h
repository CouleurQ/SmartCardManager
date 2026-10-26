#ifndef SCRIPTFILE_H
#define SCRIPTFILE_H

#include <QList>
#include <QSharedPointer>
#include <QString>

#include "commands/command.h"


// Script file ("Run script" tab): { version, dateTime, id, items: [ {name, seq, type, ...} ] }
// Schema: res/scriptFileScheme.json
class ScriptFile
{
public:
    static constexpr const char *Version = "1.0";

    // On failure false is returned, *error describes the problem and *commands is left untouched.
    static bool load(const QString &filePath, QList<QSharedPointer<Command>> *commands, QString *error);
    static bool save(const QString &filePath, const QList<QSharedPointer<Command>> &commands, QString *error);
};

#endif
