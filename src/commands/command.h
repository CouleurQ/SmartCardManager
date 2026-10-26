#ifndef COMMAND_H
#define COMMAND_H

#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QPair>
#include <QSharedPointer>
#include <QString>

#include <atomic>

class CardSession;


// A script command. Plain value type with a virtual interface: no QObject, no type enum, no casts.
class Command
{
public:
    // ordered key/value pairs: the script file keeps the keys in a human friendly order
    using JsonFields = QList<QPair<QString, QJsonValue>>;

    Command() = default;
    Command(const Command &) = default;
    Command &operator=(const Command &) = default;
    virtual ~Command() = default;

    // stable identifier, also used as "type" in the script file ("SendBytes", "SendAPDU", "TerminalCmd")
    virtual QString typeName() const = 0;
    virtual QSharedPointer<Command> clone() const = 0;
    // serialised fields without name and type
    virtual JsonFields toJson() const = 0;
    virtual void fromJson(const QJsonObject &object) = 0;
    // Runs the command on the card (blocking, not for the GUI thread); the card's answer is stored
    // in *response if given. Long waits give up when *cancel becomes true.
    virtual bool execute(CardSession &session, QByteArray *response = nullptr, const std::atomic<bool> *cancel = nullptr) const = 0;
    // the bytes sent to the card, empty for commands that do not transmit
    virtual QByteArray requestBytes() const { return QByteArray(); }

public:
    QString commandName;
    QByteArray expectedResponse;

protected:
    // true if there is no expected response or the response matches it (two bytes: status word only) (two bytes: status word only)
    bool matchesExpected(const QByteArray &response) const;
};

Q_DECLARE_METATYPE(QSharedPointer<Command>)

#endif
