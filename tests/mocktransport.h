#ifndef MOCKTRANSPORT_H
#define MOCKTRANSPORT_H

#include "card/icardtransport.h"

#include <QHash>
#include <QStringList>
#include <QThread>


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

    QStringList readers() override { return readerList; }

    bool connectCard(const QString &reader) override {
        if (!readerList.contains(reader)) {
            emit log(LogType::Error, "unknown reader");
            return false;
        }
        connectedReader = reader;
        return true;
    }

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
