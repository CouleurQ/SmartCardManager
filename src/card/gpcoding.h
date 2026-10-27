#ifndef GPCODING_H
#define GPCODING_H

#include <QByteArray>
#include <QString>
#include <QStringList>


// Decoding of GlobalPlatform Card Specification v2.3.1 codings (chapter 11.1 and 10.x).
namespace GpCoding {

enum class ObjectKind { Card, SecurityDomain, Application, ExecutableLoadFile };

// Tables 11-3 .. 11-6; Card is the life cycle state of the Issuer Security Domain
QString lifeCycleText(ObjectKind kind, quint8 value);

// Tables 11-7 .. 11-9: names of the privileges set in the three privilege bytes (missing bytes count as 0)
QStringList privilegeNames(const QByteArray &privileges);

// Table 11-16
QString keyTypeText(quint8 type);

// Section 11.1.9 (first Key Usage Qualifier byte)
QString keyUsageText(quint8 usage);

// Table 11-19
QString keyAccessText(quint8 access);

// Table 10-1 / Security Level of EXTERNAL AUTHENTICATE
QString securityLevelText(quint8 level);

}

#endif
