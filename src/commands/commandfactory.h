#ifndef COMMAND_FACTORY_H
#define COMMAND_FACTORY_H

#include "command.h"

#include <QStringList>


namespace CommandFactory {
// empty pointer for an unknown type name
QSharedPointer<Command> create(const QString &typeName);
QStringList typeNames();
}

#endif
