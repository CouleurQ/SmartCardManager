#include "cardsession.h"

#include <QElapsedTimer>
#include <QMetaObject>
#include <QMutexLocker>
#include <QThread>


namespace {
constexpr int monitorSliceMs = 100;
}


CardSession::CardSession(ICardTransport *transport, QObject *parent)
    : QObject(parent)
    , m_transport(transport)
{
    Q_ASSERT(transport != nullptr);
    m_pool.setMaxThreadCount(1);
    connect(m_transport, &ICardTransport::log, this, &CardSession::log);

    m_monitor = QThread::create([this]() { monitorLoop(); });
    m_monitor->start();
}


CardSession::~CardSession()
{
    m_stopMonitor = true;
    m_monitorWake.wakeAll();
    m_monitor->wait();
    delete m_monitor;
    m_pool.waitForDone();
}


QString CardSession::reader() const
{
    QMutexLocker lock(&m_readerMutex);
    return m_reader;
}


void CardSession::setReader(const QString &reader)
{
    {
        QMutexLocker lock(&m_readerMutex);
        m_reader = reader;
    }

    // A connection to another reader is dropped in the background. The worker compares against the
    // connected reader instead of relying on call order, so a connect that follows right away is safe.
    if (m_connected.exchange(false)) {
        m_pool.start([this, reader]() {
            {
                QMutexLocker lock(&m_mutex);
                if (m_transport->isConnected() && m_connectedReader != reader)
                    m_transport->disconnectCard();
                m_connected = m_transport->isConnected();
            }
            notifyStateChanged();
        });
    }
    emit stateChanged();
}


QStringList CardSession::readers()
{
    QMutexLocker lock(&m_mutex);
    return m_transport->readers();
}


bool CardSession::connectCard()
{
    bool ok;
    {
        QMutexLocker lock(&m_mutex);
        const QString selected = reader();
        ok = m_transport->connectCard(selected);
        if (ok)
            m_connectedReader = selected;
        m_connected = m_transport->isConnected();
        cacheAtrLocked();
    }
    m_monitorWake.wakeAll();
    notifyStateChanged();
    return ok;
}


bool CardSession::disconnectCard()
{
    bool ok;
    {
        QMutexLocker lock(&m_mutex);
        ok = m_transport->disconnectCard();
        m_connected = m_transport->isConnected();
    }
    notifyStateChanged();
    return ok;
}


bool CardSession::reset(bool cold)
{
    bool ok;
    {
        QMutexLocker lock(&m_mutex);
        ok = cold ? m_transport->coldReset() : m_transport->reset();
        m_connected = m_transport->isConnected();
        cacheAtrLocked();
    }
    notifyStateChanged();
    return ok;
}


QByteArray CardSession::transmit(const QByteArray &command)
{
    QByteArray response;
    bool lost;
    {
        QMutexLocker lock(&m_mutex);
        response = m_transport->transmit(command);
        lost = syncConnectionLocked();
    }
    if (lost)
        notifyStateChanged();
    return response;
}


bool CardSession::waitForPresence(bool present, int timeoutMs, const std::atomic<bool> *cancel)
{
    const QString selected = reader();
    if (selected.isEmpty()) {
        emit log(LogType::Error, "No reader selected");
        return false;
    }

    const ICardTransport::Presence wanted = present ? ICardTransport::Presence::Present : ICardTransport::Presence::Absent;
    ICardTransport::Presence known = ICardTransport::Presence::Unknown;
    QElapsedTimer timer;
    timer.start();

    // waitPresenceChange() is callable concurrently with other transport calls: no m_mutex here
    for (;;) {
        if (cancel && cancel->load())
            return false;
        const qint64 remaining = timeoutMs - timer.elapsed();
        if (remaining <= 0) {
            emit log(LogType::Warning, present ? "Timeout waiting for a card" : "Timeout waiting for card removal");
            return false;
        }

        const ICardTransport::Presence now = m_transport->waitPresenceChange(selected, known, static_cast<int>(qMin<qint64>(remaining, monitorSliceMs * 2)));
        if (now == wanted)
            return true;
        if (now == ICardTransport::Presence::Unknown)
            QThread::msleep(50);
        known = now;
    }
}


void CardSession::readersAsync()
{
    m_pool.start([this]() {
        const QStringList list = readers();
        QMetaObject::invokeMethod(this, [this, list]() { emit readersListed(list); }, Qt::QueuedConnection);
    });
}


void CardSession::connectCardAsync()
{
    m_pool.start([this]() { connectCard(); });
}


void CardSession::disconnectCardAsync()
{
    m_pool.start([this]() { disconnectCard(); });
}


void CardSession::resetAsync(bool cold)
{
    m_pool.start([this, cold]() { reset(cold); });
}


void CardSession::sendAsync(const QByteArray &command)
{
    m_pool.start([this, command]() {
        const QByteArray response = transmit(command);
        QMetaObject::invokeMethod(this, [this, command, response]() {
            emit responseReceived(command, response);
        }, Qt::QueuedConnection);
    });
}


void CardSession::notifyStateChanged()
{
    if (QThread::currentThread() == thread())
        emit stateChanged();
    else
        QMetaObject::invokeMethod(this, [this]() { emit stateChanged(); }, Qt::QueuedConnection);
}


QByteArray CardSession::atr() const
{
    if (!m_connected)
        return QByteArray();
    QMutexLocker lock(&m_readerMutex);
    return m_atr;
}


void CardSession::cacheAtrLocked()
{
    const QByteArray value = m_transport->isConnected() ? m_transport->atr() : QByteArray();
    QMutexLocker lock(&m_readerMutex);
    m_atr = value;
}


bool CardSession::syncConnectionLocked()
{
    const bool now = m_transport->isConnected();
    const bool lost = m_connected && !now;
    m_connected = now;
    return lost;
}


void CardSession::monitorLoop()
{
    while (!m_stopMonitor) {
        const QString watched = reader();
        if (!m_connected || watched.isEmpty()) {
            QMutexLocker lock(&m_monitorMutex);
            if (!m_stopMonitor && !m_connected)
                m_monitorWake.wait(&m_monitorMutex, 200);
            continue;
        }

        const ICardTransport::Presence now = m_transport->waitPresenceChange(watched, ICardTransport::Presence::Present, monitorSliceMs);
        if (now == ICardTransport::Presence::Unknown) {
            QThread::msleep(200);
            continue;
        }
        if (now != ICardTransport::Presence::Absent)
            continue;

        QMutexLocker lock(&m_mutex);
        // the connection may have been replaced meanwhile: only act on a still absent card
        if (!m_transport->isConnected() || m_connectedReader != watched
            || m_transport->waitPresenceChange(watched, ICardTransport::Presence::Unknown, 0) != ICardTransport::Presence::Absent)
            continue;

        m_transport->disconnectCard();
        syncConnectionLocked();
        lock.unlock();

        emit log(LogType::Warning, QString("Card removed from reader %1 - disconnected").arg(watched));
        QMetaObject::invokeMethod(this, [this]() {
            emit cardRemoved();
            emit stateChanged();
        }, Qt::QueuedConnection);
    }
}
