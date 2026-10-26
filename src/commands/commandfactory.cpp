#include "commandfactory.h"

#include "commandsendbytes.h"
#include "commandapdu.h"
#include "commandterminal.h"


QSharedPointer<Command> CommandFactory::create(const QString &typeName)
{
    if (typeName == CommandSendBytes().typeName())
        return QSharedPointer<CommandSendBytes>::create();
    if (typeName == CommandApdu().typeName())
        return QSharedPointer<CommandApdu>::create();
    if (typeName == CommandTerminal().typeName())
        return QSharedPointer<CommandTerminal>::create();
    return nullptr;
}


QStringList CommandFactory::typeNames()
{
    return {CommandSendBytes().typeName(), CommandApdu().typeName(), CommandTerminal().typeName()};
}
