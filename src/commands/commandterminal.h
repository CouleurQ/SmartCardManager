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
    enum class ActionType { WarmReset, ColdReset, Eject, Leave };
    ActionType action;
};

#endif
