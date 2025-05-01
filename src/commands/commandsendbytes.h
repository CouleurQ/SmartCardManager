#ifndef COMMAND_SEND_BYTES_H
#define COMMAND_SEND_BYTES_H

#include "command.h"


class CommandSendBytes : public Command
{
public:
    explicit CommandSendBytes(QObject *parent = nullptr)
        : Command{parent}
    {
        commandName = "New Send Bytes";
    }

    CommandSendBytes(const CommandSendBytes &copy)
        : Command(copy)
    {
        commandData = copy.commandData;
    }

    virtual Command::ObjectType type() const override
    {
        return Command::ObjectType::SendBytesType;
    }

public:
    QByteArray commandData;
};

#endif
