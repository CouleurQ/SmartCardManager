#ifndef COMMAND_H
#define COMMAND_H

#include <QObject>


class Command : public QObject
{
    Q_OBJECT

public:
    explicit Command(QObject *parent = nullptr)
        : QObject{parent}
    {
    }

    enum class ObjectType { BaseType, SendBytesType, ApduType, TerminalType };

    virtual ObjectType type() const { return ObjectType::BaseType; }

public:
    QString commandName;
    QByteArray expectedResponse;
};

#endif
