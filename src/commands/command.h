#ifndef COMMAND_H
#define COMMAND_H

#include <QObject>


class Command : public QObject
{
    Q_OBJECT

public:
    enum class ObjectType { BaseType, SendBytesType, ApduType, TerminalType };

    explicit Command(QObject *parent = nullptr)
        : QObject{parent}
    {
    }

    Command(const Command &copy)
    {
        commandName = copy.commandName;
        expectedResponse = copy.expectedResponse;
    }

    virtual ~Command() = default;

    virtual ObjectType type() const
    {
        return ObjectType::BaseType;
    }

public:
    QString commandName;
    QByteArray expectedResponse;
};

#endif
