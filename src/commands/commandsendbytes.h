#ifndef COMMAND_SEND_BYTES_H
#define COMMAND_SEND_BYTES_H

#include "command.h"


class CommandSendBytes : public Command
{
public:
    explicit CommandSendBytes(QObject *parent = nullptr)
        : Command{parent}
    {
    }

    virtual Command::ObjectType type() const { return Command::ObjectType::SendBytesType; }

public:
    QByteArray commandData;
};

#endif
