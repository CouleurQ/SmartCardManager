#include "command.h"



bool Command::matchesExpected(const QByteArray &response) const
{
    return expectedResponse.isEmpty() || response == expectedResponse;
}
