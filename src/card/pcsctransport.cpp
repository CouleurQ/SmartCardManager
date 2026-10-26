#include "pcsctransport.h"

#include <pcsclite.h>
#include <winscard.h>

#include <QMutexLocker>

#include <cstring>
#include <vector>


namespace {
QString errorText(const char *what, LONG rv)
{
    return QString("%1 (0x%2)").arg(QString::fromLatin1(what), QString::number(static_cast<quint32>(rv), 16));
}
}


PcscTransport::PcscTransport(QObject *parent) : ICardTransport(parent)
{
}


PcscTransport::~PcscTransport()
{
    if (hCard != 0)
        SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (hContext != 0)
        SCardReleaseContext(hContext);
    if (hStatusContext != 0)
        SCardReleaseContext(hStatusContext);
}


bool PcscTransport::connectCard(const QString &readerName)
{
    if (hCard != 0)
        disconnectCard();

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
    readAtr();
    return true;
}


bool PcscTransport::disconnectCard()
{
    if (hCard == 0)
        return false;

    LONG rv = SCardDisconnect(hCard, SCARD_LEAVE_CARD);
    if (rv != SCARD_S_SUCCESS)
        emit log(LogType::Error, errorText("Failed to disconnect from card", rv));

    hCard = 0;
    cardAtr.clear();
    return (rv == SCARD_S_SUCCESS);
}


bool PcscTransport::reset()
{
    return reconnect(SCARD_RESET_CARD, "Failed to reset the card");
}


bool PcscTransport::coldReset()
{
    return reconnect(SCARD_UNPOWER_CARD, "Failed to power-cycle the card");
}


bool PcscTransport::reconnect(DWORD initialization, const char *what)
{
    if (hCard == 0)
        return false;

    LONG rv = SCardReconnect(hCard, SCARD_SHARE_SHARED, SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1, initialization, &dwActiveProtocol);
    if (rv != SCARD_S_SUCCESS) {
        emit log(LogType::Error, errorText(what, rv));
        hCard = 0;
        cardAtr.clear();
        return false;
    }

    readAtr();
    return true;
}


bool PcscTransport::isConnected() const
{
    return (hCard != 0);
}


QByteArray PcscTransport::atr() const
{
    return cardAtr;
}


void PcscTransport::readAtr()
{
    cardAtr.clear();
    if (hCard == 0)
        return;

    unsigned char buffer[MAX_ATR_SIZE];
    DWORD atrLength = sizeof(buffer);
    DWORD readerLength = 0, state = 0, protocol = 0;
    if (SCardStatus(hCard, nullptr, &readerLength, &state, &protocol, buffer, &atrLength) == SCARD_S_SUCCESS)
        cardAtr = QByteArray(reinterpret_cast<const char *>(buffer), static_cast<qsizetype>(atrLength));
}


QByteArray PcscTransport::transmit(const QByteArray &command)
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
        if (rv == SCARD_W_REMOVED_CARD || rv == SCARD_E_NO_SMARTCARD || rv == SCARD_E_READER_UNAVAILABLE
            || rv == SCARD_E_NO_SERVICE || rv == SCARD_E_SERVICE_STOPPED) {
            // the connection is gone for good
            SCardDisconnect(hCard, SCARD_LEAVE_CARD);
            hCard = 0;
        }
        return QByteArray();
    }

    return QByteArray(reinterpret_cast<const char*>(recvBuffer.data()), static_cast<qsizetype>(recvLength));
}


QStringList PcscTransport::readers()
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


ICardTransport::Presence PcscTransport::waitPresenceChange(const QString &readerName, Presence known, int timeoutMs)
{
    // own context: must not interfere with the one used for the transmissions
    QMutexLocker lock(&statusMutex);
    if (hStatusContext == 0 && SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr, nullptr, &hStatusContext) != SCARD_S_SUCCESS) {
        hStatusContext = 0;
        return Presence::Unknown;
    }

    const QByteArray name = readerName.toUtf8();
    SCARD_READERSTATE state;
    std::memset(&state, 0, sizeof(state));
    state.szReader = name.constData();
    state.dwCurrentState = (known == Presence::Present) ? SCARD_STATE_PRESENT
                         : (known == Presence::Absent) ? SCARD_STATE_EMPTY : SCARD_STATE_UNAWARE;

    const LONG rv = SCardGetStatusChange(hStatusContext, known == Presence::Unknown ? 0 : static_cast<DWORD>(timeoutMs), &state, 1);

    if (rv == SCARD_E_TIMEOUT)
        return known;
    if (rv == SCARD_E_UNKNOWN_READER || rv == SCARD_E_READER_UNAVAILABLE)
        return Presence::Absent;
    if (rv != SCARD_S_SUCCESS) {
        // e.g. the service restarted: start over with a new context next time
        SCardReleaseContext(hStatusContext);
        hStatusContext = 0;
        return Presence::Unknown;
    }

    if (state.dwEventState & (SCARD_STATE_UNKNOWN | SCARD_STATE_IGNORE))
        return Presence::Absent;
    if (state.dwEventState & SCARD_STATE_UNAVAILABLE)
        return Presence::Unknown;   // e.g. reserved exclusively by another application
    return (state.dwEventState & SCARD_STATE_PRESENT) ? Presence::Present : Presence::Absent;
}


SCARDCONTEXT PcscTransport::getContext()
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
