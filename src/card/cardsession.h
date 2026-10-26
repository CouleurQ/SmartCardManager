#ifndef CARDSESSION_H
#define CARDSESSION_H

#include <QObject>
#include <QMutex>
#include <QThreadPool>
#include <QWaitCondition>

#include <atomic>

#include "icardtransport.h"


// Owns the selected reader and the connection state and serialises all access to the transport.
//
// The *Async() functions and sendAsync() run on a worker thread and report back with signals; the
// GUI uses only those, so it never waits for PC/SC. The blocking variants (connectCard(),
// transmit(), ...) are for worker threads such as the script runner. isConnected() and reader()
// never block. A monitor thread watches the card while connected and drops the connection when the
// card is removed from the reader.
class CardSession : public QObject
{
    Q_OBJECT

public:
    explicit CardSession(ICardTransport *transport, QObject *parent = nullptr);
    ~CardSession() override;

    QString reader() const;
    // selecting another reader drops an open connection (in the background)
    void setReader(const QString &reader);
    bool isConnected() const { return m_connected; }
    // ATR of the connected card (cached, never blocks); empty when not connected
    QByteArray atr() const;

    // blocking, not for the GUI thread
    QStringList readers();
    bool connectCard();
    bool disconnectCard();
    bool reset(bool cold = false);
    QByteArray transmit(const QByteArray &command);
    // waits until a card is present (or absent); false on timeout, cancel or without a reader
    bool waitForPresence(bool present, int timeoutMs, const std::atomic<bool> *cancel = nullptr);

    // non-blocking
    void readersAsync();
    void connectCardAsync();
    void disconnectCardAsync();
    void resetAsync(bool cold = false);
    void sendAsync(const QByteArray &command);

signals:
    void log(LogType type, const QString &text);
    void stateChanged();
    void readersListed(const QStringList &readers);
    void responseReceived(const QByteArray &command, const QByteArray &response);
    void cardRemoved();

private:
    void notifyStateChanged();
    // call with m_mutex held: adopts the transport state, true if the connection was lost
    bool syncConnectionLocked();
    void monitorLoop();

    ICardTransport *m_transport;
    mutable QMutex m_mutex;             // serialises the transport
    mutable QMutex m_readerMutex;       // never held during I/O
    QString m_reader;
    QString m_connectedReader;          // guarded by m_mutex
    std::atomic<bool> m_connected{false};
    QByteArray m_atr;                   // guarded by m_readerMutex
    void cacheAtrLocked();              // call with m_mutex held
    QThreadPool m_pool;

    QThread *m_monitor = nullptr;
    QMutex m_monitorMutex;
    QWaitCondition m_monitorWake;
    std::atomic<bool> m_stopMonitor{false};
};

#endif
