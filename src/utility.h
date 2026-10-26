#ifndef UTILITY_H
#define UTILITY_H

#include <QByteArray>
#include <QString>


namespace Utility {

// Ignores every non hex character; an odd digit count is padded with a trailing 0.
QByteArray hexStringToByteArray(const QString &hexString);

// "00 A4 04" (upper case, space separated)
QString byteArrayToHexString(const QByteArray &data);

}

#endif
