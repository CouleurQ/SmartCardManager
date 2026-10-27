#ifndef MOCKTRANSPORT_H
#define MOCKTRANSPORT_H

#include "card/icardtransport.h"

#include <QElapsedTimer>
#include <QHash>
#include <QStringList>
#include <QThread>

#include <atomic>


// Scripted card transport for tests: a fixed reader list and canned answers per command.
class MockTransport : public ICardTransport
{
    Q_OBJECT

public:
    using ICardTransport::ICardTransport;

    QStringList readerList{"Mock Reader 0", "Mock Reader 1"};
    QHash<QByteArray, QByteArray> answers;      // command -> response
    QByteArray defaultAnswer = QByteArray::fromHex("9000");
    int transmitDelayMs = 0;

    QString connectedReader;
    QList<QByteArray> sent;
    int resetCount = 0;
    int coldResetCount = 0;
    std::atomic<bool> cardInserted{true};
    QByteArray cardAtr = QByteArray::fromHex("3B6800000073C84013009000");

    QStringList readers() override { return readerList; }

    bool connectCard(const QString &reader) override {
        if (!readerList.contains(reader)) {
            emit log(LogType::Error, "unknown reader");
            return false;
        }
        if (!cardInserted) {
            emit log(LogType::Error, "no card in reader");
            return false;
        }
        connectedReader = reader;
        return true;
    }

    QByteArray atr() const override { return cardAtr; }

    bool disconnectCard() override {
        const bool was = !connectedReader.isEmpty();
        connectedReader.clear();
        return was;
    }

    bool reset() override {
        if (connectedReader.isEmpty())
            return false;
        ++resetCount;
        return true;
    }

    bool coldReset() override {
        if (connectedReader.isEmpty())
            return false;
        ++coldResetCount;
        return true;
    }

    Presence waitPresenceChange(const QString &, Presence known, int timeoutMs) override {
        const Presence now = cardInserted ? Presence::Present : Presence::Absent;
        if (known == Presence::Unknown)
            return now;
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < timeoutMs) {
            const Presence p = cardInserted ? Presence::Present : Presence::Absent;
            if (p != known)
                return p;
            QThread::msleep(5);
        }
        return known;
    }

    bool isConnected() const override { return !connectedReader.isEmpty(); }

    QByteArray transmit(const QByteArray &command) override {
        if (transmitDelayMs > 0)
            QThread::msleep(transmitDelayMs);
        sent.append(command);
        if (connectedReader.isEmpty()) {
            emit log(LogType::Error, "Card not connected");
            return QByteArray();
        }
        return answers.value(command, defaultAnswer);
    }
};

#endif
