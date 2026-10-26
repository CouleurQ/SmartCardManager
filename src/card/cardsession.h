#ifndef CARDSESSION_H
#define CARDSESSION_H

#include <QObject>
#include <QMutex>
#include <QThreadPool>

#include "icardtransport.h"


// Owns the selected reader and the connection state and serialises all access to the transport.
// Blocking calls are meant for short operations; use sendAsync() for transmissions so that a
// stalling card does not freeze the GUI.
class CardSession : public QObject
{
    Q_OBJECT

public:
    explicit CardSession(ICardTransport *transport, QObject *parent = nullptr);
    ~CardSession() override;

    QStringList readers();
    QString reader() const { return m_reader; }
    // selecting another reader drops an open connection
    void setReader(const QString &reader);

    bool isConnected() const;
    bool connectCard();
    bool disconnectCard();
    bool reset();
    QByteArray transmit(const QByteArray &command);
    void sendAsync(const QByteArray &command);

signals:
    void log(LogType type, const QString &text);
    void stateChanged();
    void responseReceived(const QByteArray &command, const QByteArray &response);

private:
    ICardTransport *m_transport;
    QString m_reader;
    mutable QMutex m_mutex;
    QThreadPool m_pool;
};

#endif
