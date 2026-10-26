#include "commandapdu.h"

#include "card/cardsession.h"
#include "utility.h"

#include <QJsonObject>


QSharedPointer<Command> CommandApdu::clone() const
{
    return QSharedPointer<CommandApdu>::create(*this);
}


Command::JsonFields CommandApdu::toJson() const
{
    return {
        {"cla", Utility::byteArrayToHexString(cla)},
        {"ins", Utility::byteArrayToHexString(ins)},
        {"p1", Utility::byteArrayToHexString(p1)},
        {"p2", Utility::byteArrayToHexString(p2)},
        {"data", Utility::byteArrayToHexString(commandData)},
        {"le", Utility::byteArrayToHexString(le)},
        {"expectedResponse", Utility::byteArrayToHexString(expectedResponse)},
    };
}


void CommandApdu::fromJson(const QJsonObject &object)
{
    cla = Utility::hexStringToByteArray(object["cla"].toString());
    ins = Utility::hexStringToByteArray(object["ins"].toString());
    p1 = Utility::hexStringToByteArray(object["p1"].toString());
    p2 = Utility::hexStringToByteArray(object["p2"].toString());
    // "cmd" is accepted for files written by older versions
    commandData = Utility::hexStringToByteArray(object["data"].toString(object["cmd"].toString()));
    le = Utility::hexStringToByteArray(object["le"].toString());
    expectedResponse = Utility::hexStringToByteArray(object["expectedResponse"].toString());
}


QByteArray CommandApdu::buildApdu() const
{
    QByteArray apdu = cla.left(1).leftJustified(1, '\0') + ins.left(1).leftJustified(1, '\0')
                      + p1.left(1).leftJustified(1, '\0') + p2.left(1).leftJustified(1, '\0');

    const qsizetype length = commandData.size();
    if (length > 255) {
        apdu.append('\0');
        apdu.append(static_cast<char>((length >> 8) & 0xFF));
        apdu.append(static_cast<char>(length & 0xFF));
    } else if (length > 0) {
        apdu.append(static_cast<char>(length));
    }
    apdu.append(commandData);
    apdu.append(le);
    return apdu;
}


bool CommandApdu::execute(CardSession &session, QByteArray *response, const std::atomic<bool> *cancel) const
{
    const QByteArray answer = session.transmit(buildApdu());
    if (response)
        *response = answer;
    return !answer.isEmpty() && matchesExpected(answer);
}
