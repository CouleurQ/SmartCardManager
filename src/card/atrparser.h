#ifndef ATRPARSER_H
#define ATRPARSER_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>


// Decodes an Answer To Reset (ISO/IEC 7816-3, historical bytes after ISO/IEC 7816-4 and PC/SC part 3).
namespace AtrParser {

enum class Kind { Ts, T0, Interface, Historical, Tck };

struct Row {
    int first = -1;             // byte range in the ATR, -1 for none
    int count = 0;
    Kind kind = Kind::Interface;
    QString field;
    QString meaning;
    QList<Row> children;
};

struct Info {
    QByteArray raw;
    bool valid = false;         // structure could be decoded completely
    QStringList summary;        // short human readable facts
    QStringList warnings;
    QList<Row> rows;
    QList<Kind> byteKinds;      // one entry per raw byte
};

Info parse(const QByteArray &atr);

}

#endif
