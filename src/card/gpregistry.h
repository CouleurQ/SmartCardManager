#ifndef GPREGISTRY_H
#define GPREGISTRY_H

#include <QByteArray>
#include <QList>
#include <QString>

#include "gpcoding.h"


// One entry of the GlobalPlatform Registry as returned by GET STATUS (Tables 11-36 and 11-37).
struct GpRegistryEntry
{
    enum class Kind { IssuerSecurityDomain, SecurityDomain, Application, ExecutableLoadFile };

    Kind kind = Kind::Application;
    QByteArray aid;                     // '4F'
    quint8 lifeCycle = 0;               // '9F70'
    QByteArray privileges;              // 'C5' (3 bytes), not for load files
    QByteArray loadFileAid;             // 'C4': Executable Load File of an application
    QByteArray domainAid;               // 'CC': associated Security Domain
    QByteArray version;                 // 'CE': Executable Load File version number
    QList<QByteArray> moduleAids;       // '84': Executable Modules of a load file
    QString label;                      // well known name, if any (not part of the registry)

    GpCoding::ObjectKind codingKind() const
    {
        switch (kind) {
        case Kind::IssuerSecurityDomain: return GpCoding::ObjectKind::Card;
        case Kind::SecurityDomain: return GpCoding::ObjectKind::SecurityDomain;
        case Kind::ExecutableLoadFile: return GpCoding::ObjectKind::ExecutableLoadFile;
        case Kind::Application: break;
        }
        return GpCoding::ObjectKind::Application;
    }
};


// One 'C0' entry of the Key Information Template 'E0' (Tables 11-28 and 11-29).
struct GpKeyInfo
{
    struct Component {
        quint8 type = 0;                // Table 11-16
        int length = 0;                 // in bytes
    };

    quint8 id = 0;
    quint8 version = 0;
    QList<Component> components;
    bool hasUsage = false;              // extended format only
    quint8 usage = 0;
    bool hasAccess = false;
    quint8 access = 0;
};


// A data object read with GET DATA.
struct GpDataObject
{
    QByteArray tag;
    QString name;
    QByteArray value;                   // BER-TLV value (without tag and length)
    QString interpretation;             // decoded form, may be empty
};


struct GpCardContent
{
    QList<GpRegistryEntry> registry;
    QList<GpDataObject> dataObjects;
    QList<GpKeyInfo> keys;
};

#endif
