#ifndef COMMAND_APDU_H
#define COMMAND_APDU_H

#include "command.h"


class CommandApdu : public Command
{
public:
    explicit CommandApdu(QObject *parent = nullptr)
        : Command{parent}
    {
    }

    virtual Command::ObjectType type() const { return Command::ObjectType::SendBytesType; }

public:
    unsigned char cla;
    unsigned char ins;
    unsigned char p1;
    unsigned char p2;
    QByteArray commandData;
    unsigned char le;
};

#endif
