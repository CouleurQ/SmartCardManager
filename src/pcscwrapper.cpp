#include "pcscwrapper.h"

#include <pcsclite.h>
#include <winscard.h>


PcscWrapper::PcscWrapper(QObject* parent) : QObject(parent)
{
}


bool PcscWrapper::connect(const QString &readerName)
{
    LONG rv = SCardConnect(getContext(), readerName.toUtf8().constData(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &hCard, &dwActiveProtocol);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, QString("%1 (0x%2)").arg("Failed to connect the card", QString::number(rv, 16)));
    }

    return (rv == SCARD_S_SUCCESS);
}


bool PcscWrapper::disconnect(const QString &readerName)
{
    LONG rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, QString("%1 (0x%2)").arg("Failed to disconnect from card", QString::number(rv, 16)));
    }

    hCard = 0;
    return (rv == SCARD_S_SUCCESS);
}


bool PcscWrapper::reset(const QString &readerName)
{
    if (hCard == 0)
        return false;

    LONG rv = SCardReconnect(hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, SCARD_RESET_CARD, &dwActiveProtocol);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, QString("%1 (0x%2)").arg("Failed to reset the card", QString::number(rv, 16)));
        hCard = 0;
        return rv;
    }

    return (rv == SCARD_S_SUCCESS);
}


bool PcscWrapper::isConnected(const QString &readerName)
{
    return (hCard != 0);
}


QByteArray PcscWrapper::send(const QString &readerName, const QByteArray command)
{
    if (hCard == 0)
        return QByteArray();

    const unsigned char *sendBuffer = reinterpret_cast<const unsigned char*>(command.constData());
    DWORD sendLength = command.size();

    unsigned char recvBuffer[258];
    DWORD recvLength = sizeof(recvBuffer);

    const SCARD_IO_REQUEST *pioSendPci = nullptr;
    if (dwActiveProtocol == SCARD_PROTOCOL_T0)
        pioSendPci = SCARD_PCI_T0;
    else if (dwActiveProtocol == SCARD_PROTOCOL_T1)
        pioSendPci = SCARD_PCI_T1;

    LONG rv = SCardTransmit(hCard, pioSendPci, sendBuffer, sendLength, NULL, recvBuffer, &recvLength);

    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, QString("%1 (0x%2)").arg("Failed to send command", QString::number(rv, 16)));
    }

    return QByteArray(reinterpret_cast<char*>(recvBuffer), recvLength);
}


QStringList PcscWrapper::getReaders()
{
    QStringList cardReaderList;

    // Get the list of readers
    DWORD dwReaders = 0;
    LONG rv = SCardListReaders(getContext(), NULL, NULL, &dwReaders);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, QString("%1 (0x%2)").arg("Failed to get reader list size", QString::number(rv, 16)));
        return QStringList();
    }

    char *mszReaders = new char[dwReaders];
    rv = SCardListReaders(getContext(), nullptr, mszReaders, &dwReaders);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, QString("%1 (0x%2)").arg("Failed to get reader list", QString::number(rv, 16)));
        return QStringList();
    }

    // Parse the readers into a QStringList
    char *reader = mszReaders;
    while (*reader != '\0') {
        cardReaderList += QString::fromUtf8(reader);
        reader += strlen(reader) + 1;
    }

    // Clean up
    SCardFreeMemory(getContext(), mszReaders);

    return cardReaderList;
}


SCARDCONTEXT PcscWrapper::getContext()
{
    static SCARDCONTEXT hContext = 0;

    if (hContext != 0)
        return hContext;

    LONG ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr, nullptr, &hContext);
    if (ret != SCARD_S_SUCCESS) {
        emit log(LogType::Error, QString("%1 (0x%2)").arg("Failed to establish context - PCSC service not running?", QString::number(ret, 16)));
        return 0;
    }

    return hContext;
}

