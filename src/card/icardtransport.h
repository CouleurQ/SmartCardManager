#ifndef ICARDTRANSPORT_H
#define ICARDTRANSPORT_H

#include <QObject>
#include <QByteArray>
#include <QStringList>

#include "global.h"


// Abstraction of a smart card reader backend (PC/SC in production, a mock in tests).
// Everything except waitPresenceChange() is serialised by CardSession and may run on a worker
// thread; waitPresenceChange() may be called concurrently with all other methods.
class ICardTransport : public QObject
{
    Q_OBJECT

public:
    using QObject::QObject;

    enum class Presence { Unknown, Absent, Present };

    virtual QStringList readers() = 0;
    virtual bool connectCard(const QString &reader) = 0;
    virtual bool disconnectCard() = 0;
    virtual bool reset() = 0;
    // power-cycles the card; transports without the distinction fall back to the warm reset
    virtual bool coldReset() { return reset(); }
    virtual bool isConnected() const = 0;
    // ATR of the connected card, empty if unknown
    virtual QByteArray atr() const { return QByteArray(); }
    // empty result means failure
    virtual QByteArray transmit(const QByteArray &command) = 0;

    // Returns the card presence in the reader. Without a known state (Unknown) it answers at once;
    // otherwise it blocks up to timeoutMs until the presence differs from `known` and returns
    // `known` on timeout. A vanished reader counts as Absent, undeterminable states as Unknown.
    virtual Presence waitPresenceChange(const QString &reader, Presence known, int timeoutMs) = 0;

signals:
    void log(LogType type, const QString &text);
};

#endif
