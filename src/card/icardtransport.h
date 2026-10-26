#ifndef ICARDTRANSPORT_H
#define ICARDTRANSPORT_H

#include <QObject>
#include <QByteArray>
#include <QStringList>

#include "global.h"


// Abstraction of a smart card reader backend (PC/SC in production, a mock in tests).
class ICardTransport : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    virtual QStringList readers() = 0;
    virtual bool connectCard(const QString &reader) = 0;
    virtual bool disconnectCard() = 0;
    virtual bool reset() = 0;
    virtual bool isConnected() const = 0;
    // empty result means failure
    virtual QByteArray transmit(const QByteArray &command) = 0;

signals:
    void log(LogType type, const QString &text);
};

#endif
