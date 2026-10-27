#ifndef GPAUTHPARAMS_H
#define GPAUTHPARAMS_H

#include <QByteArray>
#include <QString>


// Everything needed to open a GlobalPlatform secure channel to a Security Domain
// (SELECT, INITIALIZE UPDATE + EXTERNAL AUTHENTICATE for SCP01/02/03; SCP11 per Amendment F).
struct GpAuthParams
{
    enum class Protocol { Scp01, Scp02, Scp03, Scp11a, Scp11b, Scp11c };
    enum class KeySource { DefaultTestKeys, StaticKeys, MasterKey };
    enum class Diversification { None, Visa2, EmvCps11 };

    static constexpr quint8 DefaultTestKey[16] = {
        0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F };

    Protocol protocol = Protocol::Scp03;
    int iParameter = -1;                // "i" of the protocol; -1: as announced by the card
    quint8 securityLevel = 0x01;        // P1 of EXTERNAL AUTHENTICATE

    bool selectFirst = true;
    QByteArray targetAid = QByteArray::fromHex("A000000151000000");

    quint8 keyVersion = 0;              // 0: first available key set
    quint8 keyId = 0;

    KeySource keySource = KeySource::DefaultTestKeys;
    QByteArray encKey;
    QByteArray macKey;
    QByteArray dekKey;
    QByteArray masterKey;
    Diversification diversification = Diversification::None;

    QByteArray hostChallenge;           // 8 bytes; empty: random

    QString oceCertificateFile;         // SCP11a/c
    QString oceKeyFile;

    bool readRegistry = true;

    // Secure Channel Protocol identifier as announced in the Card Recognition Data (Annex H)
    quint8 scpIdentifier() const
    {
        switch (protocol) {
        case Protocol::Scp01: return 0x01;
        case Protocol::Scp02: return 0x02;
        case Protocol::Scp03: return 0x03;
        default: return 0x11;
        }
    }

    bool isScp11() const { return scpIdentifier() == 0x11; }

    static QString protocolText(Protocol p)
    {
        switch (p) {
        case Protocol::Scp01: return "SCP01";
        case Protocol::Scp02: return "SCP02";
        case Protocol::Scp03: return "SCP03";
        case Protocol::Scp11a: return "SCP11a";
        case Protocol::Scp11b: return "SCP11b";
        case Protocol::Scp11c: return "SCP11c";
        }
        return QString();
    }
};

#endif
