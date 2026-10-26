#include "commandterminal.h"

#include "card/cardsession.h"

#include <QJsonObject>


QSharedPointer<Command> CommandTerminal::clone() const
{
    return QSharedPointer<CommandTerminal>::create(*this);
}


Command::JsonFields CommandTerminal::toJson() const
{
    return {
        {"action", getActionString(action)},
    };
}


void CommandTerminal::fromJson(const QJsonObject &object)
{
    action = ActionType::None;
    const QString text = object["action"].toString();
    for (int i = 0; i < static_cast<int>(ActionType::_count_); ++i) {
        if (getActionString(static_cast<ActionType>(i)) == text)
            action = static_cast<ActionType>(i);
    }
}


bool CommandTerminal::execute(CardSession &session, QByteArray *response) const
{
    if (response)
        response->clear();

    switch (action) {
    case ActionType::WarmReset:
    case ActionType::ColdReset:
        return session.reset();
    case ActionType::Connect:
        return session.connectCard();
    case ActionType::Disconnect:
        return session.disconnectCard();
    case ActionType::WaitForCard:
    case ActionType::WaitForRemoval:
    case ActionType::None:
    default:
        return false;   // not supported by the transport yet
    }
}
