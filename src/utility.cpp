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
