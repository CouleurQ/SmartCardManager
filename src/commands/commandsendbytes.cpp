#include "commandsendbytes.h"

#include "card/cardsession.h"
#include "utility.h"

#include <QJsonObject>


QSharedPointer<Command> CommandSendBytes::clone() const
{
    return QSharedPointer<CommandSendBytes>::create(*this);
}


Command::JsonFields CommandSendBytes::toJson() const
{
    return {
        {"cmd", Utility::byteArrayToHexString(commandData)},
        {"expectedResponse", Utility::byteArrayToHexString(expectedResponse)},
    };
}


void CommandSendBytes::fromJson(const QJsonObject &object)
{
    // "data" is accepted for files written by older versions
    commandData = Utility::hexStringToByteArray(object["cmd"].toString(object["data"].toString()));
    expectedResponse = Utility::hexStringToByteArray(object["expectedResponse"].toString());
}


bool CommandSendBytes::execute(CardSession &session, QByteArray *response) const
{
    const QByteArray answer = session.transmit(commandData);
    if (response)
        *response = answer;
    return !answer.isEmpty() && matchesExpected(answer);
}
