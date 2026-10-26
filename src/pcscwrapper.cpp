#include "pcscwrapper.h"

#include <pcsclite.h>
#include <winscard.h>

#include <cstring>
#include <vector>


namespace {
QString errorText(const char *what, LONG rv)
{
    return QString("%1 (0x%2)").arg(QString::fromLatin1(what), QString::number(static_cast<quint32>(rv), 16));
}
}


PcscWrapper::PcscWrapper(QObject* parent) : QObject(parent)
{
}


PcscWrapper::~PcscWrapper()
{
    if (hCard != 0)
        SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (hContext != 0)
        SCardReleaseContext(hContext);
}


bool PcscWrapper::connect(const QString &readerName)
{
    if (hCard != 0)
        disconnect(readerName);

    SCARDCONTEXT context = getContext();
    if (context == 0)
        return false;

    SCARDHANDLE card = 0;
    DWORD protocol = 0;
    LONG rv = SCardConnect(context, readerName.toUtf8().constData(), SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, &card, &protocol);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, errorText("Failed to connect the card", rv));
        return false;
    }

    hCard = card;
    dwActiveProtocol = protocol;
    return true;
}


bool PcscWrapper::disconnect(const QString &)
{
    if (hCard == 0)
        return false;

    LONG rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (rv != SCARD_S_SUCCESS)
        emit log(LogType::Error, errorText("Failed to disconnect from card", rv));

    hCard = 0;
    return (rv == SCARD_S_SUCCESS);
}


bool PcscWrapper::reset(const QString &)
{
    if (hCard == 0)
        return false;

    LONG rv = SCardReconnect(hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, SCARD_RESET_CARD, &dwActiveProtocol);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, errorText("Failed to reset the card", rv));
        hCard = 0;
        return false;
    }

    return true;
}


bool PcscWrapper::isConnected(const QString &)
{
    return (hCard != 0);
}


QByteArray PcscWrapper::send(const QString &, const QByteArray command)
{
    if (hCard == 0) {
        emit log(LogType::Error, "Card not connected");
        return QByteArray();
    }

    const SCARD_IO_REQUEST *pioSendPci = SCARD_PCI_RAW;
    if (dwActiveProtocol == SCARD_PROTOCOL_T0)
        pioSendPci = SCARD_PCI_T0;
    else if (dwActiveProtocol == SCARD_PROTOCOL_T1)
        pioSendPci = SCARD_PCI_T1;

    // large enough for extended length responses (65535 + SW1 SW2)
    std::vector<unsigned char> recvBuffer(65538, 0);
    DWORD recvLength = static_cast<DWORD>(recvBuffer.size());

    LONG rv = SCardTransmit(hCard, pioSendPci, reinterpret_cast<const unsigned char*>(command.constData()),
                            static_cast<DWORD>(command.size()), nullptr, recvBuffer.data(), &recvLength);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, errorText("Failed to send command", rv));
        return QByteArray();
    }

    return QByteArray(reinterpret_cast<const char*>(recvBuffer.data()), static_cast<qsizetype>(recvLength));
}


QStringList PcscWrapper::getReaders()
{
    SCARDCONTEXT context = getContext();
    if (context == 0)
        return QStringList();

    DWORD dwReaders = 0;
    LONG rv = SCardListReaders(context, nullptr, nullptr, &dwReaders);
    if (rv == SCARD_E_NO_READERS_AVAILABLE) {
        emit log(LogType::Warning, "No PC/SC readers connected");
        return QStringList();
    } else if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, errorText("Failed to get reader list size", rv));
        return QStringList();
    }

    std::vector<char> mszReaders(dwReaders, '\0');
    rv = SCardListReaders(context, nullptr, mszReaders.data(), &dwReaders);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, errorText("Failed to get reader list", rv));
        return QStringList();
    }

    // multi-string: "reader1\0reader2\0\0"
    QStringList cardReaderList;
    const char *reader = mszReaders.data();
    while (*reader != '\0') {
        cardReaderList += QString::fromUtf8(reader);
        reader += strlen(reader) + 1;
    }

    return cardReaderList;
}


SCARDCONTEXT PcscWrapper::getContext()
{
    if (hContext != 0)
        return hContext;

    LONG rv = SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr, nullptr, &hContext);
    if (rv != SCARD_S_SUCCESS) {
        hContext = 0;
        emit log(LogType::Error, errorText("Failed to establish context - PCSC service not running?", rv));
        return 0;
    }

    return hContext;
}
