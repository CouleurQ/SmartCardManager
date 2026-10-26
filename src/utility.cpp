#include "utility.h"

#include <QRegularExpression>


QByteArray Utility::hexStringToByteArray(const QString &hexString)
{
    static const QRegularExpression regEx = QRegularExpression(QStringLiteral("[^0-9a-fA-F]"));
    QString string = hexString;

    string.remove(regEx);

    if (string.length() % 2 != 0)
        string += "0";

    return QByteArray::fromHex(string.toLatin1());
}


QString Utility::byteArrayToHexString(const QByteArray &data)
{
    return QString::fromLatin1(data.toHex(' ').toUpper());
}


QString Utility::statusWordText(quint8 sw1, quint8 sw2)
{
    const quint16 sw = static_cast<quint16>((sw1 << 8) | sw2);

    switch (sw) {
    case 0x9000: return "Success";
    case 0x6200: return "Warning: no information given, state unchanged";
    case 0x6281: return "Warning: part of the returned data may be corrupted";
    case 0x6282: return "Warning: end of file or record reached before Le bytes were read";
    case 0x6283: return "Warning: selected file or application invalidated (locked)";
    case 0x6284: return "Warning: FCI not formatted according to ISO 7816-4";
    case 0x6300: return "Warning: no information given, state changed";
    case 0x6310: return "Warning: more data available";
    case 0x6381: return "Warning: file filled up by the last write";
    case 0x6400: return "Execution error: state unchanged";
    case 0x6500: return "Execution error: state changed";
    case 0x6581: return "Execution error: memory failure";
    case 0x6700: return "Wrong length";
    case 0x6800: return "Function in CLA not supported";
    case 0x6881: return "Logical channel not supported";
    case 0x6882: return "Secure messaging not supported";
    case 0x6883: return "Last command of the chain expected";
    case 0x6884: return "Command chaining not supported";
    case 0x6900: return "Command not allowed";
    case 0x6981: return "Command incompatible with file structure";
    case 0x6982: return "Security status not satisfied";
    case 0x6983: return "Authentication method blocked";
    case 0x6984: return "Referenced data invalidated";
    case 0x6985: return "Conditions of use not satisfied";
    case 0x6986: return "Command not allowed (no current EF)";
    case 0x6987: return "Expected secure messaging data objects missing";
    case 0x6988: return "Secure messaging data objects incorrect";
    case 0x6A80: return "Incorrect parameters in the command data field";
    case 0x6A81: return "Function not supported / card locked";
    case 0x6A82: return "File or application not found";
    case 0x6A83: return "Record not found";
    case 0x6A84: return "Not enough memory space";
    case 0x6A85: return "Nc inconsistent with TLV structure";
    case 0x6A86: return "Incorrect parameters P1-P2";
    case 0x6A87: return "Nc inconsistent with P1-P2";
    case 0x6A88: return "Referenced data not found";
    case 0x6A89: return "File already exists";
    case 0x6A8A: return "DF name already exists";
    case 0x6B00: return "Wrong parameters P1-P2";
    case 0x6D00: return "Instruction code not supported or invalid";
    case 0x6E00: return "Class not supported";
    case 0x6F00: return "No precise diagnosis";
    default: break;
    }

    if (sw1 == 0x61)
        return QString("Success: %1 more byte(s) available (GET RESPONSE)").arg(sw2);
    if (sw1 == 0x6C)
        return QString("Wrong Le: exact length is %1").arg(sw2);
    if (sw1 == 0x63 && (sw2 & 0xF0) == 0xC0)
        return QString("Verification failed: %1 attempt(s) left").arg(sw2 & 0x0F);
    if (sw1 == 0x62)
        return "Warning: state unchanged";
    if (sw1 == 0x63)
        return "Warning: state changed";
    if (sw1 == 0x64)
        return "Execution error: state unchanged";
    if (sw1 == 0x65)
        return "Execution error: state changed";
    if (sw1 == 0x66)
        return "Security related issue";
    if (sw1 >= 0x67 && sw1 <= 0x6F)
        return "Checking error";
    if (sw1 == 0x90 || (sw1 & 0xF0) == 0x90)
        return "Proprietary status";
    return "Unknown status word";
}
