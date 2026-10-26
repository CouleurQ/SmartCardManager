#ifndef COMMAND_APDU_H
#define COMMAND_APDU_H

#include "command.h"


class CommandApdu : public Command
{
public:
    CommandApdu() { commandName = "New APDU command"; }

    QString typeName() const override { return QStringLiteral("SendAPDU"); }
    QSharedPointer<Command> clone() const override;
    JsonFields toJson() const override;
    void fromJson(const QJsonObject &object) override;
    bool execute(CardSession &session, QByteArray *response = nullptr) const override;

    // CLA INS P1 P2 [Lc data] [Le]; data longer than 255 bytes uses the extended Lc form
    QByteArray buildApdu() const;

public:
    QByteArray cla;
    QByteArray ins;
    QByteArray p1;
    QByteArray p2;
    QByteArray commandData;
    QByteArray le;
};

#endif
