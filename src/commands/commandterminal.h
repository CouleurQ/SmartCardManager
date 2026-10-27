#ifndef COMMAND_TERMINAL_H
#define COMMAND_TERMINAL_H

#include "command.h"


class CommandTerminal : public Command
{
public:
    CommandTerminal() { commandName = "New Terminal Command"; }

    QString typeName() const override { return QStringLiteral("TerminalCmd"); }
    QSharedPointer<Command> clone() const override;
    JsonFields toJson() const override;
    void fromJson(const QJsonObject &object) override;
    bool execute(CardSession &session, QByteArray *response = nullptr, const std::atomic<bool> *cancel = nullptr) const override;

public:
    enum class ActionType { None = 0, WarmReset, ColdReset, WaitForCard, WaitForRemoval, Connect, Disconnect, _count_ };

    ActionType action = ActionType::None;
    static constexpr int waitTimeoutMs = 30000;   // WaitForCard / WaitForRemoval give up after this

    static QString getActionString(const ActionType value) {
        switch (value) {
        case ActionType::WarmReset: return "WarmReset";
        case ActionType::ColdReset: return "ColdReset";
        case ActionType::WaitForCard: return "WaitForCard";
        case ActionType::WaitForRemoval: return "WaitForRemoval";
        case ActionType::Connect: return "Connect";
        case ActionType::Disconnect: return "Disconnect";
        default: ;
        }
        return QString("");
    }
};

#endif
