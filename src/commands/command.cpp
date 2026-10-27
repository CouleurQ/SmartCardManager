#include "command.h"



bool Command::matchesExpected(const QByteArray &response) const
{
    if (expectedResponse.isEmpty())
        return true;
    // a two byte expectation is a status word and is compared with the SW of the response only
    if (expectedResponse.size() == 2)
        return response.size() >= 2 && response.right(2) == expectedResponse;
    return response == expectedResponse;
}
