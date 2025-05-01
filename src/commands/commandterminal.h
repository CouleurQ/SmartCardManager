#ifndef COMMAND_TERMINAL_H
#define COMMAND_TERMINAL_H

#include "command.h"


class CommandTerminal : public Command
{
public:
    explicit CommandTerminal(QObject *parent = nullptr)
        : Command{parent}
    {
        commandName = "New Terminal Command";
        action = ActionType::None;
    }

    CommandTerminal(const CommandTerminal &copy)
        : Command(copy)
    {
        action = copy.action;
    }

    virtual Command::ObjectType type() const override
    {
        return Command::ObjectType::TerminalType;
    }

public:
    enum class ActionType { None, WarmReset = 0, ColdReset, WaitForCard, WaitForRemoval, Connect, Disconnect, _count_ };

    ActionType action;

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
