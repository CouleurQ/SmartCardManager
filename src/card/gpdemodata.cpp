#include "gpdemodata.h"

#include "utility.h"

namespace GpDemoData {

namespace {

QByteArray hex(const char *text)
{
    return QByteArray::fromHex(text);
}


QString hex2(quint8 value)
{
    return QString("%1").arg(value, 2, 16, QChar('0')).toUpper();
}


QByteArray tagBytes(quint16 tag)
{
    QByteArray out;
    if (tag > 0xFF)
        out.append(char(tag >> 8));
    out.append(char(tag & 0xFF));
    return out;
}


QByteArray tlv(quint16 tag, const QByteArray &value)
{
    QByteArray out = tagBytes(tag);
    if (value.size() >= 0x80)
        out.append(char(0x81));
    out.append(char(value.size()));
    return out + value;
}


GpRegistryEntry domain(GpRegistryEntry::Kind kind, const char *aid, quint8 lifeCycle, const char *privileges,
                       const char *parent, const QString &label)
{
    GpRegistryEntry e;
    e.kind = kind;
    e.aid = hex(aid);
    e.lifeCycle = lifeCycle;
    e.privileges = hex(privileges);
    e.domainAid = hex(parent);
    e.label = label;
    return e;
}


GpRegistryEntry application(const char *aid, quint8 lifeCycle, const char *privileges, const char *loadFile,
                            const char *parent, const QString &label)
{
    GpRegistryEntry e = domain(GpRegistryEntry::Kind::Application, aid, lifeCycle, privileges, parent, label);
    e.loadFileAid = hex(loadFile);
    return e;
}


GpRegistryEntry loadFile(const char *aid, const char *version, const char *parent, const QString &label,
                         const QList<QByteArray> &modules = {})
{
    GpRegistryEntry e;
    e.kind = GpRegistryEntry::Kind::ExecutableLoadFile;
    e.aid = hex(aid);
    e.lifeCycle = 0x01;
    e.version = hex(version);
    e.domainAid = hex(parent);
    e.moduleAids = modules;
    e.label = label;
    return e;
}


QList<GpRegistryEntry> registry()
{
    const char *isd = "A000000151000000";
    const char *ssd = "A000000151535041";
    const char *wallet = "F00102030405";

    QList<GpRegistryEntry> list;
    list << domain(GpRegistryEntry::Kind::IssuerSecurityDomain, isd, 0x0F, "9E7C80", "", "Issuer Security Domain");
    list << domain(GpRegistryEntry::Kind::SecurityDomain, ssd, 0x0F, "804000", isd, "Supplementary Security Domain");
    list << domain(GpRegistryEntry::Kind::SecurityDomain, "F000000001000001", 0x07, "800000", isd, QString());

    list << application("F0010203040501", 0x07, "000000", wallet, isd, "Wallet");
    list << application("F0010203040502", 0x03, "000000", wallet, isd, QString());
    list << application("A000000063504B43532D3135", 0x07, "000000", "A00000006350", ssd, "PKCS#15");
    list << application("F0010203040A01", 0x83, "000000", wallet, ssd, "Wallet (locked)");

    list << loadFile("A0000001515350", "0100", isd, "GlobalPlatform Security Domain",
                     {hex("A000000151000000"), hex("A000000151535041")});
    list << loadFile("A0000000620001", "0106", isd, "java.lang");
    list << loadFile("A0000000620101", "0106", isd, "javacard.framework");
    list << loadFile("A0000000620102", "0106", isd, "javacard.security");
    list << loadFile("A0000000620201", "0106", isd, "javacardx.crypto");
    list << loadFile("A00000006350", "0100", isd, "PKCS#15 package", {hex("A000000063504B43532D3135")});
    list << loadFile(wallet, "0102", isd, "com.example.wallet",
                     {hex("F0010203040501"), hex("F0010203040502"), hex("F0010203040A01")});
    return list;
}


QList<GpKeyInfo> keys()
{
    auto key = [](quint8 id, quint8 version, quint8 type, int length, int usage = -1) {
        GpKeyInfo k;
        k.id = id;
        k.version = version;
        k.components.append({type, length});
        if (usage >= 0) {
            k.hasUsage = true;
            k.usage = quint8(usage);
            k.hasAccess = true;
            k.access = 0x00;
        }
        return k;
    };

    QList<GpKeyInfo> list;
    list << key(0x01, 0x01, 0x80, 16) << key(0x02, 0x01, 0x80, 16) << key(0x03, 0x01, 0x80, 16);
    list << key(0x01, 0x30, 0x88, 16, 0x18) << key(0x02, 0x30, 0x88, 16, 0x14) << key(0x03, 0x30, 0x88, 16, 0x48);
    list << key(0x11, 0x10, 0xB1, 32, 0x42);
    return list;
}


QByteArray keyInformationTemplate(const QList<GpKeyInfo> &keys)
{
    QByteArray all;
    for (const GpKeyInfo &k : keys) {
        QByteArray data;
        data.append(char(k.id));
        data.append(char(k.version));
        for (const GpKeyInfo::Component &c : k.components) {
            data.append(char(c.type));
            data.append(char(c.length));
        }
        all += tlv(0xC0, data);
    }
    return all;
}


QString keyInformationText(const QList<GpKeyInfo> &keys)
{
    QStringList lines;
    for (const GpKeyInfo &k : keys) {
        const GpKeyInfo::Component &c = k.components.first();
        lines << QString("KVN %1, KID %2: %3, %4 bytes")
                     .arg(hex2(k.version), hex2(k.id), GpCoding::keyTypeText(c.type))
                     .arg(c.length);
    }
    return lines.join('\n');
}


int defaultIParameter(GpAuthParams::Protocol protocol)
{
    switch (protocol) {
    case GpAuthParams::Protocol::Scp03: return 0x70;
    case GpAuthParams::Protocol::Scp02: return 0x55;
    case GpAuthParams::Protocol::Scp01: return 0x05;
    default: return 0x00;
    }
}


// Card Recognition Data (Annex H, Table H-1) without the '66' header
QByteArray cardRecognitionData(const GpAuthParams &auth)
{
    const quint8 i = quint8(auth.iParameter >= 0 ? auth.iParameter : defaultIParameter(auth.protocol));

    const QByteArray gp = hex("2A864886FC6B");             // OID 1.2.840.114283
    QByteArray scp = gp + hex("04");
    scp.append(char(auth.scpIdentifier()));
    scp.append(char(i));

    QByteArray inner = tlv(0x06, gp + hex("01"));
    inner += tlv(0x60, tlv(0x06, gp + hex("020203")));
    inner += tlv(0x63, tlv(0x06, gp + hex("03")));
    inner += tlv(0x64, tlv(0x06, scp));
    return tlv(0x73, inner);
}


QByteArray applicationList(const QList<GpRegistryEntry> &entries)
{
    QByteArray all;
    for (const GpRegistryEntry &e : entries) {
        if (e.kind != GpRegistryEntry::Kind::ExecutableLoadFile)
            all += tlv(0x61, tlv(0x4F, e.aid));
    }
    return all;
}


QString aidList(const QList<GpRegistryEntry> &entries)
{
    QStringList lines;
    for (const GpRegistryEntry &e : entries) {
        if (e.kind != GpRegistryEntry::Kind::ExecutableLoadFile)
            lines << Utility::byteArrayToHexString(e.aid).remove(' ');
    }
    return lines.join('\n');
}


GpDataObject object(quint16 tag, const QString &name, const QByteArray &value, const QString &interpretation = QString())
{
    return {tagBytes(tag), name, value, interpretation};
}

} // namespace


GpCardContent content(const GpAuthParams &auth)
{
    GpCardContent c;
    c.registry = registry();
    c.keys = keys();

    // GET DATA reports the security level of the open secure channel with the AUTHENTICATED indicator
    const quint8 level = quint8(0x80 | auth.securityLevel);

    const quint8 scpI = quint8(auth.iParameter >= 0 ? auth.iParameter : defaultIParameter(auth.protocol));
    const QByteArray capability = tlv(0xA0, tlv(0x80, QByteArray(1, char(auth.scpIdentifier())))
                                            + tlv(0x81, QByteArray(1, char(scpI))))
                                + tlv(0x82, hex("020000"));
    const QByteArray resources = tlv(0x81, hex("00000007")) + tlv(0x82, hex("00025800")) + tlv(0x83, hex("00001800"));

    c.dataObjects << object(0x42, "Issuer Identification Number (IIN)", hex("290000001234"));
    c.dataObjects << object(0x45, "Card Image Number (CIN)", hex("0102030405060708"));
    c.dataObjects << object(0x66, "Card Data", cardRecognitionData(auth),
                            QString("Card Recognition Data: GlobalPlatform card specification 2.3, %1, i=%2")
                                .arg(GpAuthParams::protocolText(auth.protocol), hex2(scpI)));
    c.dataObjects << object(0x67, "Card Capability Information", capability,
                            QString("Secure Channel Protocol %1, i=%2\nPrivileges assignable to applications: %3")
                                .arg(hex2(auth.scpIdentifier()), hex2(scpI),
                                     GpCoding::privilegeNames(hex("020000")).join(", ")));
    c.dataObjects << object(0xC1, "Sequence Counter of the default Key Version Number", hex("002A"), "42");
    c.dataObjects << object(0xC2, "Confirmation Counter", hex("0005"), "5");
    c.dataObjects << object(0xD3, "Current Security Level", QByteArray(1, char(level)),
                            GpCoding::securityLevelText(level));
    c.dataObjects << object(0xE0, "Key Information Template", keyInformationTemplate(c.keys),
                            keyInformationText(c.keys));
    c.dataObjects << object(0x2F00, "List of Applications", applicationList(c.registry), aidList(c.registry));
    c.dataObjects << object(0x5F50, "Security Domain Manager URL", QByteArray("https://sdm.example.org/ssd"),
                            "https://sdm.example.org/ssd");
    c.dataObjects << object(0xFF21, "Extended Card Resources Information", resources,
                            "Installed applications: 7\nFree non-volatile memory: 153600 bytes\nFree volatile memory: 6144 bytes");
    return c;
}

} // namespace GpDemoData
