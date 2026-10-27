#include "gpcoding.h"

namespace GpCoding {

namespace {

QString unknown(quint8 value)
{
    return QString("Unknown (%1)").arg(QString("%1").arg(value, 2, 16, QChar('0')).toUpper());
}

} // namespace


QString lifeCycleText(ObjectKind kind, quint8 v)
{
    switch (kind) {
    case ObjectKind::Card:
        switch (v) {
        case 0x01: return "OP_READY";
        case 0x07: return "INITIALIZED";
        case 0x0F: return "SECURED";
        case 0x7F: return "CARD_LOCKED";
        case 0xFF: return "TERMINATED";
        }
        break;
    case ObjectKind::ExecutableLoadFile:
        if (v == 0x01)
            return "LOADED";
        break;
    case ObjectKind::SecurityDomain:
        if (v == 0x03) return "INSTALLED";
        if (v == 0x07) return "SELECTABLE";
        if (v == 0x0F) return "PERSONALIZED";
        if ((v & 0xF3) == 0x83) return "LOCKED";
        break;
    case ObjectKind::Application:
        if (v == 0x03) return "INSTALLED";
        if (v == 0x07) return "SELECTABLE";
        if ((v & 0x83) == 0x83) return "LOCKED";
        if ((v & 0x87) == 0x07)
            return QString("Application specific (%1)").arg(QString("%1").arg(v, 2, 16, QChar('0')).toUpper());
        break;
    }
    return unknown(v);
}


QStringList privilegeNames(const QByteArray &privileges)
{
    const quint8 b1 = privileges.size() > 0 ? quint8(privileges[0]) : 0;
    const quint8 b2 = privileges.size() > 1 ? quint8(privileges[1]) : 0;
    const quint8 b3 = privileges.size() > 2 ? quint8(privileges[2]) : 0;

    QStringList names;
    if (b1 & 0x80) names << "Security Domain";
    if ((b1 & 0xC1) == 0xC0) names << "DAP Verification";
    if ((b1 & 0xA0) == 0xA0) names << "Delegated Management";
    if (b1 & 0x10) names << "Card Lock";
    if (b1 & 0x08) names << "Card Terminate";
    if (b1 & 0x04) names << "Card Reset";
    if (b1 & 0x02) names << "CVM Management";
    if ((b1 & 0xC1) == 0xC1) names << "Mandated DAP Verification";

    if (b2 & 0x80) names << "Trusted Path";
    if (b2 & 0x40) names << "Authorized Management";
    if (b2 & 0x20) names << "Token Management";
    if (b2 & 0x10) names << "Global Delete";
    if (b2 & 0x08) names << "Global Lock";
    if (b2 & 0x04) names << "Global Registry";
    if (b2 & 0x02) names << "Final Application";
    if (b2 & 0x01) names << "Global Service";

    if (b3 & 0x80) names << "Receipt Generation";
    if (b3 & 0x40) names << "Ciphered Load File Data Block";
    if (b3 & 0x20) names << "Contactless Activation";
    if (b3 & 0x10) names << "Contactless Self-Activation";
    return names;
}


QString keyTypeText(quint8 t)
{
    if (t < 0x80) return "Private use";
    switch (t) {
    case 0x80: return "DES";
    case 0x85: return "Pre-Shared Key (TLS)";
    case 0x88: return "AES";
    case 0x90: return "HMAC-SHA1";
    case 0x91: return "HMAC-SHA1-160";
    case 0xA0: return "RSA public key: exponent e";
    case 0xA1: return "RSA public key: modulus N";
    case 0xA2: return "RSA private key: modulus N";
    case 0xA3: return "RSA private key: exponent d";
    case 0xA4: return "RSA private key (CRT): P";
    case 0xA5: return "RSA private key (CRT): Q";
    case 0xA6: return "RSA private key (CRT): PQ";
    case 0xA7: return "RSA private key (CRT): DP1";
    case 0xA8: return "RSA private key (CRT): DQ1";
    case 0xB0: return "ECC public key";
    case 0xB1: return "ECC private key";
    case 0xB2: return "ECC field parameter P";
    case 0xB3: return "ECC field parameter A";
    case 0xB4: return "ECC field parameter B";
    case 0xB5: return "ECC field parameter G";
    case 0xB6: return "ECC field parameter N";
    case 0xB7: return "ECC field parameter k";
    case 0xF0: return "ECC key parameters reference";
    case 0xFF: return "Extended format";
    }
    return unknown(t);
}


QString keyUsageText(quint8 u)
{
    switch (u) {
    case 0x14: return "C-MAC";
    case 0x24: return "R-MAC";
    case 0x34: return "C-MAC + R-MAC";
    case 0x18: return "C-ENC";
    case 0x28: return "R-ENC";
    case 0x38: return "C-ENC + R-ENC";
    case 0x48: return "C-DEK";
    case 0x88: return "R-DEK";
    case 0xC8: return "C-DEK + R-DEK";
    case 0x82: return "PK_SD_AUT";
    case 0x42: return "SK_SD_AUT";
    case 0x81: return "Token";
    case 0x44: return "Receipt";
    case 0x84: return "DAP";
    case 0x83: return "PK_SD_AUT + Token";
    case 0x43: return "SK_SD_AUT + Receipt";
    case 0x86: return "PK_SD_AUT + DAP";
    case 0x87: return "PK_SD_AUT + Token + DAP";
    }
    return unknown(u);
}


QString keyAccessText(quint8 a)
{
    if (a == 0x00) return "Security Domain and associated Applications";
    if (a == 0x01) return "Security Domain only";
    if (a == 0x02) return "Associated Applications only";
    if (a == 0xFF) return "Not available";
    if (a >= 0x20) return "Proprietary";
    return unknown(a);
}


QString securityLevelText(quint8 level)
{
    if (level == 0x00)
        return "NO_SECURITY_LEVEL";

    QStringList parts;
    if ((level & 0xC0) == 0x80) parts << "AUTHENTICATED";
    if ((level & 0xC0) == 0x40) parts << "ANY_AUTHENTICATED";
    if (level & 0x01) parts << "C_MAC";
    if (level & 0x02) parts << "C_DECRYPTION";
    if (level & 0x10) parts << "R_MAC";
    if (level & 0x20) parts << "R_ENCRYPTION";
    return parts.isEmpty() ? unknown(level) : parts.join(" + ");
}

} // namespace GpCoding
