#include "atrparser.h"

#include "utility.h"

#include <QMap>

using namespace AtrParser;


namespace {

const int fiTable[16] = {372, 372, 558, 744, 1116, 1488, 1860, 0, 0, 512, 768, 1024, 1536, 2048, 0, 0};
const double fMaxTable[16] = {4, 5, 6, 8, 12, 16, 20, 0, 0, 5, 7.5, 10, 15, 20, 0, 0};
const int diTable[16] = {0, 1, 2, 4, 8, 16, 32, 64, 12, 20, 0, 0, 0, 0, 0, 0};


QString hex(const QByteArray &data, int first, int count)
{
    return Utility::byteArrayToHexString(data.mid(first, count));
}


QString hex1(quint8 value)
{
    return QString("%1").arg(value, 2, 16, QChar('0')).toUpper();
}


QString printable(const QByteArray &data)
{
    QString text;
    for (char c : data)
        text += (c >= 0x20 && c < 0x7F) ? QChar(c) : QChar('.');
    return text;
}


QString number(double value)
{
    return QString::number(value, 'g', 6);
}


QString lifeCycle(quint8 lcs)
{
    if (lcs == 0x00) return "no information given";
    if (lcs == 0x01) return "creation state";
    if (lcs == 0x03) return "initialisation state";
    if (lcs == 0x05 || lcs == 0x07) return "operational state (activated)";
    if (lcs == 0x04 || lcs == 0x06) return "operational state (deactivated)";
    if ((lcs & 0xFC) == 0x0C) return "termination state";
    return "proprietary / reserved";
}


QString contactlessStandard(quint8 ss)
{
    static const QMap<quint8, QString> names = {
        {0x01, "ISO 14443 A, part 1"}, {0x02, "ISO 14443 A, part 2"}, {0x03, "ISO 14443 A, part 3"},
        {0x05, "ISO 14443 B, part 1"}, {0x06, "ISO 14443 B, part 2"}, {0x07, "ISO 14443 B, part 3"},
        {0x09, "ISO 15693, part 1"}, {0x0A, "ISO 15693, part 2"}, {0x0B, "ISO 15693, part 3"},
        {0x0C, "ISO 15693, part 4"}, {0x11, "FeliCa"}, {0x12, "Topaz / Jewel"},
    };
    return names.value(ss, QString("standard 0x%1").arg(hex1(ss)));
}


QString contactlessCard(quint16 nn)
{
    static const QMap<quint16, QString> names = {
        {0x0001, "MIFARE Classic 1K"}, {0x0002, "MIFARE Classic 4K"}, {0x0003, "MIFARE Ultralight"},
        {0x0026, "MIFARE Mini"}, {0x003A, "MIFARE Ultralight C"}, {0xF004, "Topaz / Jewel"},
    };
    return names.value(nn, QString("card name 0x%1").arg(nn, 4, 16, QChar('0')).toUpper());
}


// Compact-TLV historical bytes (ISO 7816-4, category indicator 0x80 / 0x00 / 0x10)
void parseHistorical(const QByteArray &atr, int first, int count, Info &info)
{
    if (count == 0)
        return;

    Row group;
    group.first = first;
    group.count = count;
    group.kind = Kind::Historical;
    group.field = QString("Historical bytes (%1)").arg(count);

    const QByteArray bytes = atr.mid(first, count);
    group.meaning = QString("\"%1\"").arg(printable(bytes));

    const quint8 category = static_cast<quint8>(bytes[0]);
    Row cat;
    cat.first = first;
    cat.count = 1;
    cat.kind = Kind::Historical;
    cat.field = "Category indicator";

    int pos = 1;
    int end = count;
    bool tlv = false;

    if (category == 0x80 || category == 0x00) {
        tlv = true;
        cat.meaning = category == 0x80 ? "0x80 - compact-TLV data objects follow"
                                       : "0x00 - compact-TLV data objects, followed by the status information";
        if (category == 0x00 && count >= 4) {
            end = count - 3;   // LCS + SW1 SW2 at the end
        }
    } else if (category == 0x10) {
        cat.meaning = "0x10 - DIR data reference follows";
        if (count >= 3) {
            Row dir;
            dir.first = first + 1;
            dir.count = count - 1;
            dir.kind = Kind::Historical;
            dir.field = "DIR data reference";
            dir.meaning = hex(atr, first + 1, count - 1);
            group.children.append(cat);
            group.children.append(dir);
            info.rows.append(group);
            info.summary.append("Historical bytes: DIR data reference");
            return;
        }
    } else {
        cat.meaning = QString("0x%1 - proprietary format").arg(hex1(category));
        group.children.append(cat);
        Row prop;
        prop.first = first + 1;
        prop.count = count - 1;
        prop.kind = Kind::Historical;
        prop.field = "Proprietary data";
        prop.meaning = QString("%1  \"%2\"").arg(hex(atr, first + 1, count - 1), printable(bytes.mid(1)));
        if (count > 1)
            group.children.append(prop);
        info.rows.append(group);
        info.summary.append(QString("Historical bytes: proprietary (\"%1\")").arg(printable(bytes)));
        return;
    }
    group.children.append(cat);

    // PC/SC part 3 contactless ATR: 80 4F 0C <RID 5> <SS> <NN NN> <RFU 4>
    if (category == 0x80 && count >= 15 && static_cast<quint8>(bytes[1]) == 0x4F && static_cast<quint8>(bytes[2]) == 0x0C
        && bytes.mid(3, 5) == QByteArray::fromHex("A000000306")) {
        Row aid;
        aid.first = first + 1;
        aid.count = 2 + 5;
        aid.kind = Kind::Historical;
        aid.field = "Application identifier (PC/SC)";
        aid.meaning = QString("%1 - PC/SC workgroup RID A0 00 00 03 06").arg(hex(atr, first + 1, 7));
        group.children.append(aid);
        Row ss;
        ss.first = first + 8;
        ss.count = 1;
        ss.kind = Kind::Historical;
        ss.field = "Standard";
        ss.meaning = contactlessStandard(static_cast<quint8>(bytes[8]));
        group.children.append(ss);
        const quint16 nn = (static_cast<quint8>(bytes[9]) << 8) | static_cast<quint8>(bytes[10]);
        Row name;
        name.first = first + 9;
        name.count = 2;
        name.kind = Kind::Historical;
        name.field = "Card name";
        name.meaning = contactlessCard(nn);
        group.children.append(name);
        info.summary.append(QString("Contactless card: %1 (%2)").arg(contactlessCard(nn), contactlessStandard(static_cast<quint8>(bytes[8]))));
        if (count > 11) {
            Row rest;
            rest.first = first + 11;
            rest.count = count - 11;
            rest.kind = Kind::Historical;
            rest.field = "Reserved for future use";
            rest.meaning = hex(atr, first + 11, count - 11);
            group.children.append(rest);
        }
        info.rows.append(group);
        return;
    }

    while (tlv && pos < end) {
        const quint8 head = static_cast<quint8>(bytes[pos]);
        const int tag = head >> 4;
        const int len = head & 0x0F;
        if (pos + 1 + len > end) {
            info.warnings.append("Historical bytes: compact-TLV object exceeds the data");
            break;
        }
        const QByteArray value = bytes.mid(pos + 1, len);
        Row row;
        row.first = first + pos;
        row.count = 1 + len;
        row.kind = Kind::Historical;
        const QString raw = Utility::byteArrayToHexString(value);

        switch (tag) {
        case 0x1:
            row.field = "Country code";
            row.meaning = raw;
            break;
        case 0x2:
            row.field = "Issuer identification number";
            row.meaning = raw;
            break;
        case 0x3:
            row.field = "Card service data";
            if (len == 1) {
                const quint8 v = static_cast<quint8>(value[0]);
                QStringList parts;
                if (v & 0x80) parts << "application selection by full DF name";
                if (v & 0x40) parts << "application selection by partial DF name";
                if (v & 0x20) parts << "BER-TLV data objects in EF.DIR";
                if (v & 0x10) parts << "BER-TLV data objects in EF.ATR";
                const int access = (v >> 1) & 0x07;
                if (access == 0x04) parts << "EF.DIR/EF.ATR via READ BINARY";
                else if (access == 0x02) parts << "EF.DIR/EF.ATR via READ RECORD";
                else if (access == 0x00) parts << "EF.DIR/EF.ATR via GET DATA";
                parts << ((v & 0x01) ? "card has an MF" : "card has no MF");
                row.meaning = QString("%1: %2").arg(raw, parts.join(", "));
            } else {
                row.meaning = raw;
            }
            break;
        case 0x4:
            row.field = "Initial access data";
            row.meaning = raw;
            break;
        case 0x5:
            row.field = "Card issuer's data";
            row.meaning = QString("%1  \"%2\"").arg(raw, printable(value));
            break;
        case 0x6:
            row.field = "Pre-issuing data";
            row.meaning = raw;
            break;
        case 0x7:
            row.field = "Card capabilities";
            if (len >= 1) {
                const quint8 v = static_cast<quint8>(value[0]);
                QStringList parts;
                if (v & 0x80) parts << "DF selection by full name";
                if (v & 0x40) parts << "DF selection by partial name";
                if (v & 0x20) parts << "DF selection by path";
                if (v & 0x10) parts << "DF selection by file identifier";
                if (v & 0x08) parts << "implicit DF selection";
                if (v & 0x04) parts << "short EF identifier supported";
                if (v & 0x02) parts << "record number supported";
                if (v & 0x01) parts << "record identifier supported";
                if (len >= 3) {
                    const quint8 c = static_cast<quint8>(value[2]);
                    if (c & 0x80) parts << "command chaining";
                    if (c & 0x40) parts << "extended Lc/Le";
                    parts << QString("up to %1 logical channel(s)").arg((c & 0x07) + 1);
                }
                row.meaning = QString("%1: %2").arg(raw, parts.join(", "));
            } else {
                row.meaning = raw;
            }
            break;
        case 0x8:
            row.field = "Status indicator";
            if (len == 1) {
                row.meaning = QString("%1: life cycle - %2").arg(raw, lifeCycle(static_cast<quint8>(value[0])));
            } else if (len == 2) {
                row.meaning = QString("%1: SW %2").arg(raw, QString(raw).remove(' '));
            } else if (len == 3) {
                row.meaning = QString("%1: life cycle - %2, SW %3").arg(raw, lifeCycle(static_cast<quint8>(value[0])),
                                                                         Utility::byteArrayToHexString(value.mid(1)).remove(' '));
            } else {
                row.meaning = raw;
            }
            break;
        case 0xF:
            row.field = "Application identifier";
            row.meaning = raw;
            break;
        default:
            row.field = QString("Object, tag %1").arg(tag, 0, 16).toUpper();
            row.meaning = raw;
        }

        group.children.append(row);
        pos += 1 + len;
    }

    if (category == 0x00 && count >= 4) {
        Row status;
        status.first = first + end;
        status.count = 3;
        status.kind = Kind::Historical;
        status.field = "Status information";
        status.meaning = QString("life cycle - %1, SW %2").arg(lifeCycle(static_cast<quint8>(bytes[end])),
                                                                hex(atr, first + end + 1, 2).remove(' '));
        group.children.append(status);
    }

    info.summary.append(QString("Historical bytes: %1 byte(s), \"%2\"").arg(count).arg(printable(bytes)));
    info.rows.append(group);
}

} // namespace


Info AtrParser::parse(const QByteArray &atr)
{
    Info info;
    info.raw = atr;
    info.byteKinds.fill(Kind::Interface, atr.size());

    if (atr.size() < 2) {
        info.warnings.append(atr.isEmpty() ? "No ATR available" : "ATR too short (at least TS and T0 are required)");
        return info;
    }
    if (atr.size() > 33)
        info.warnings.append("ATR is longer than the 33 bytes allowed by ISO/IEC 7816-3");

    // --- TS ---
    const quint8 ts = static_cast<quint8>(atr[0]);
    info.byteKinds[0] = Kind::Ts;
    Row tsRow;
    tsRow.first = 0;
    tsRow.count = 1;
    tsRow.kind = Kind::Ts;
    tsRow.field = "TS - initial character";
    if (ts == 0x3B) {
        tsRow.meaning = "0x3B - direct convention (high = 1, MSB first)";
        info.summary.append("Direct convention");
    } else if (ts == 0x3F) {
        tsRow.meaning = "0x3F - inverse convention (high = 0, LSB first)";
        info.summary.append("Inverse convention");
    } else {
        tsRow.meaning = QString("0x%1 - invalid, expected 3B or 3F").arg(hex1(ts));
        info.warnings.append("Invalid initial character TS");
    }
    info.rows.append(tsRow);

    // --- T0 ---
    const quint8 t0 = static_cast<quint8>(atr[1]);
    info.byteKinds[1] = Kind::T0;
    const int historicalCount = t0 & 0x0F;
    Row t0Row;
    t0Row.first = 1;
    t0Row.count = 1;
    t0Row.kind = Kind::T0;
    t0Row.field = "T0 - format character";
    t0Row.meaning = QString("%1 historical byte(s); interface bytes present:").arg(historicalCount);
    {
        QStringList present;
        if (t0 & 0x10) present << "TA1";
        if (t0 & 0x20) present << "TB1";
        if (t0 & 0x40) present << "TC1";
        if (t0 & 0x80) present << "TD1";
        t0Row.meaning += present.isEmpty() ? " none" : " " + present.join(", ");
    }
    info.rows.append(t0Row);

    // --- interface bytes ---
    Row interfaceGroup;
    interfaceGroup.kind = Kind::Interface;
    interfaceGroup.field = "Interface bytes";
    interfaceGroup.first = 2;

    int pos = 2;
    quint8 y = t0 >> 4;
    int level = 1;
    int protocol = 0;               // protocol selected by TD(level-1), for level >= 3
    QList<int> protocols;           // protocols offered, in order
    bool structureOk = true;
    bool needTck = false;
    int fi = 372, di = 1;
    double fMax = 4;
    bool haveTa1 = false;

    auto addRow = [&](Row row, int at) {
        row.first = at;
        row.count = 1;
        row.kind = Kind::Interface;
        info.byteKinds[at] = Kind::Interface;
        interfaceGroup.children.append(row);
    };

    while (true) {
        auto take = [&](quint8 mask, quint8 &value) -> int {
            if (!(y & mask))
                return -1;
            if (pos >= atr.size()) {
                structureOk = false;
                return -1;
            }
            value = static_cast<quint8>(atr[pos]);
            return pos++;
        };

        quint8 ta = 0, tb = 0, tc = 0, td = 0;
        const int taAt = take(0x1, ta);
        const int tbAt = take(0x2, tb);
        const int tcAt = take(0x4, tc);
        const int tdAt = take(0x8, td);

        const bool globalLevel = (level == 1 || level == 2 || protocol == 15);
        const QString lv = QString::number(level);

        if (taAt >= 0) {
            Row r;
            r.field = QString("TA%1").arg(lv);
            if (level == 1) {
                const int fiIdx = ta >> 4, diIdx = ta & 0x0F;
                fi = fiTable[fiIdx];
                di = diTable[diIdx];
                fMax = fMaxTable[fiIdx];
                haveTa1 = true;
                if (fi == 0 || di == 0) {
                    r.meaning = QString("0x%1 - Fi/Di index reserved for future use").arg(hex1(ta));
                    info.warnings.append("TA1 uses reserved Fi/Di values");
                    fi = 372;
                    di = 1;
                    fMax = 4;
                } else {
                    r.meaning = QString("0x%1 - Fi = %2, Di = %3 (max. clock %4 MHz, %5 clock cycles per bit)")
                                    .arg(hex1(ta)).arg(fi).arg(di).arg(number(fMax)).arg(number(static_cast<double>(fi) / di));
                }
            } else if (level == 2) {
                r.field = "TA2 - specific mode";
                const int t = ta & 0x0F;
                r.meaning = QString("0x%1 - %2, %3, protocol T=%4")
                                .arg(hex1(ta),
                                     (ta & 0x80) ? "negotiation not possible (card cannot change mode)" : "card can change mode",
                                     (ta & 0x10) ? "implicit parameters" : "parameters from the interface bytes")
                                .arg(t);
            } else if (protocol == 15) {
                r.field = QString("TA%1 - global").arg(lv);
                const int clk = ta >> 6;
                const QString clkText = clk == 0 ? "clock stop not supported" : clk == 1 ? "clock stop, state L"
                                       : clk == 2 ? "clock stop, state H" : "clock stop, no preference";
                QStringList classes;
                if (ta & 0x01) classes << "A (5 V)";
                if (ta & 0x02) classes << "B (3 V)";
                if (ta & 0x04) classes << "C (1.8 V)";
                r.meaning = QString("0x%1 - %2; operating classes: %3").arg(hex1(ta), clkText, classes.isEmpty() ? "-" : classes.join(", "));
                info.summary.append(QString("%1; classes %2").arg(clkText, classes.isEmpty() ? "-" : classes.join(", ")));
            } else if (protocol == 1) {
                r.field = QString("TA%1 - IFSC").arg(lv);
                r.meaning = (ta == 0x00 || ta == 0xFF) ? QString("0x%1 - reserved").arg(hex1(ta))
                                                       : QString("0x%1 - information field size of the card: %2 bytes").arg(hex1(ta)).arg(ta);
                info.summary.append(QString("T=1 IFSC: %1 bytes").arg(ta));
            } else {
                r.meaning = QString("0x%1 - protocol T=%2 specific").arg(hex1(ta)).arg(protocol);
            }
            addRow(r, taAt);
        }

        if (tbAt >= 0) {
            Row r;
            r.field = QString("TB%1").arg(lv);
            if (level == 1) {
                r.meaning = tb == 0 ? "0x00 - no programming voltage required"
                                               : QString("0x%1 - programming voltage (deprecated, ignored by modern cards)").arg(hex1(tb));
            } else if (level == 2) {
                r.meaning = QString("0x%1 - programming voltage PI2 (deprecated)").arg(hex1(tb));
            } else if (protocol == 1) {
                r.field = QString("TB%1 - BWI / CWI").arg(lv);
                r.meaning = QString("0x%1 - block waiting time index BWI = %2, character waiting time index CWI = %3")
                                .arg(hex1(tb)).arg(tb >> 4).arg(tb & 0x0F);
            } else {
                r.meaning = QString("0x%1 - protocol specific").arg(hex1(tb));
            }
            addRow(r, tbAt);
        }

        if (tcAt >= 0) {
            Row r;
            r.field = QString("TC%1").arg(lv);
            if (level == 1) {
                r.field = "TC1 - extra guard time N";
                r.meaning = tc == 0xFF ? "0xFF - minimum guard time (T=0: 12 etu, T=1: 11 etu)"
                                       : QString("0x%1 - %2 additional etu between characters").arg(hex1(tc)).arg(tc);
            } else if (level == 2 && (protocols.isEmpty() || protocols.first() == 0)) {
                r.field = "TC2 - work waiting time WI";
                r.meaning = tc == 0 ? "0x00 - reserved" : QString("0x%1 - WI = %2").arg(hex1(tc)).arg(tc);
            } else if (protocol == 1 && !globalLevel) {
                r.field = QString("TC%1 - error detection").arg(lv);
                r.meaning = QString("0x%1 - %2").arg(hex1(tc), (tc & 1) ? "CRC" : "LRC");
            } else {
                r.meaning = QString("0x%1 - protocol specific").arg(hex1(tc));
            }
            addRow(r, tcAt);
        }

        if (tdAt >= 0) {
            const int p = td & 0x0F;
            Row r;
            r.field = QString("TD%1").arg(lv);
            QStringList next;
            if (td & 0x10) next << QString("TA%1").arg(level + 1);
            if (td & 0x20) next << QString("TB%1").arg(level + 1);
            if (td & 0x40) next << QString("TC%1").arg(level + 1);
            if (td & 0x80) next << QString("TD%1").arg(level + 1);
            r.meaning = QString("0x%1 - protocol T=%2%3; following: %4")
                            .arg(hex1(td)).arg(p)
                            .arg(p == 15 ? " (global interface bytes)" : "")
                            .arg(next.isEmpty() ? "none" : next.join(", "));
            addRow(r, tdAt);
            if (p != 15 && !protocols.contains(p))
                protocols.append(p);
            if (p != 0)
                needTck = true;
            protocol = p;
            y = td >> 4;
            ++level;
            if (level > 8) {
                structureOk = false;
                break;
            }
            continue;
        }
        break;
    }
    if (!structureOk) {
        info.warnings.append("ATR ends inside the interface bytes");
    }
    if (protocols.isEmpty())
        protocols.append(0);   // T=0 is the default

    if (!interfaceGroup.children.isEmpty()) {
        interfaceGroup.count = pos - 2;
        info.rows.append(interfaceGroup);
    }

    // --- historical bytes ---
    const int histFirst = pos;
    int histCount = historicalCount;
    if (histFirst + histCount > atr.size()) {
        info.warnings.append("ATR is shorter than announced by T0 (historical bytes truncated)");
        histCount = qMax(0, atr.size() - histFirst);
        structureOk = false;
    }
    for (int i = 0; i < histCount; ++i)
        info.byteKinds[histFirst + i] = Kind::Historical;
    parseHistorical(atr, histFirst, histCount, info);

    // --- TCK ---
    int next = histFirst + histCount;
    if (needTck) {
        if (next < atr.size()) {
            quint8 x = 0;
            for (int i = 1; i <= next; ++i)
                x ^= static_cast<quint8>(atr[i]);
            const quint8 tck = static_cast<quint8>(atr[next]);
            info.byteKinds[next] = Kind::Tck;
            Row r;
            r.first = next;
            r.count = 1;
            r.kind = Kind::Tck;
            r.field = "TCK - check character";
            const bool ok = (x == 0);
            r.meaning = QString("0x%1 - %2").arg(hex1(tck), ok ? "checksum valid" : "checksum INVALID (XOR of T0..TCK is not 0)");
            info.rows.append(r);
            if (!ok)
                info.warnings.append("TCK checksum mismatch");
            ++next;
        } else {
            info.warnings.append("TCK is missing although a protocol other than T=0 is offered");
            structureOk = false;
        }
    }
    if (next < atr.size())
        info.warnings.append(QString("%1 unexpected byte(s) after the end of the ATR").arg(atr.size() - next));

    // --- summary ---
    QStringList protoText;
    for (int p : protocols)
        protoText << QString("T=%1").arg(p);
    info.summary.prepend(QString("Protocol%1: %2%3").arg(protocols.size() > 1 ? "s" : "", protoText.join(", "),
                                                         protocols.size() > 1 ? QString(" (first offered: T=%1)").arg(protocols.first()) : QString()));
    if (haveTa1) {
        info.summary.insert(1, QString("Speed: Fi = %1, Di = %2, up to %3 MHz, %4 kbit/s at 4 MHz")
                                   .arg(fi).arg(di).arg(number(fMax)).arg(number(4000.0 * di / fi)));
    } else {
        info.summary.insert(1, "Speed: default (Fi = 372, Di = 1, 4 MHz, about 10.75 kbit/s)");
    }

    info.valid = structureOk;
    return info;
}
