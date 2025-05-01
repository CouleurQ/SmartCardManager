#ifndef COMMAND_APDU_H
#define COMMAND_APDU_H

#include "command.h"


class CommandApdu : public Command
{
public:
    explicit CommandApdu(QObject *parent = nullptr)
        : Command{parent}
    {
        commandName = "New APDU command";
    }

    CommandApdu(const CommandApdu &copy)
        : Command(copy)
    {
        cla = copy.cla;
        ins = copy.ins;
        p1 = copy.p1;
        p2 = copy.p2;
        commandData = copy.commandData;
        le = copy.le;
    }

    virtual Command::ObjectType type() const override
    {
        return Command::ObjectType::ApduType;
    }

public:
    QByteArray cla;
    QByteArray ins;
    QByteArray p1;
    QByteArray p2;
    QByteArray commandData;
    QByteArray le;
};

#endif
