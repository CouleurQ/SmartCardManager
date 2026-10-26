#ifndef COMMAND_SEND_BYTES_H
#define COMMAND_SEND_BYTES_H

#include "command.h"


class CommandSendBytes : public Command
{
public:
    CommandSendBytes() { commandName = "New Send Bytes"; }

    QString typeName() const override { return QStringLiteral("SendBytes"); }
    QSharedPointer<Command> clone() const override;
    JsonFields toJson() const override;
    void fromJson(const QJsonObject &object) override;
    bool execute(CardSession &session, QByteArray *response = nullptr) const override;

public:
    QByteArray commandData;
};

#endif
