#include "cardsession.h"

#include <QMetaObject>
#include <QMutexLocker>


CardSession::CardSession(ICardTransport *transport, QObject *parent)
    : QObject(parent)
    , m_transport(transport)
{
    Q_ASSERT(transport != nullptr);
    m_pool.setMaxThreadCount(1);
    connect(m_transport, &ICardTransport::log, this, &CardSession::log);
}


CardSession::~CardSession()
{
    m_pool.waitForDone();
}


QStringList CardSession::readers()
{
    QMutexLocker lock(&m_mutex);
    return m_transport->readers();
}


void CardSession::setReader(const QString &reader)
{
    {
        QMutexLocker lock(&m_mutex);
        if (m_transport->isConnected())
            m_transport->disconnectCard();
        m_reader = reader;
    }
    emit stateChanged();
}


bool CardSession::isConnected() const
{
    QMutexLocker lock(&m_mutex);
    return m_transport->isConnected();
}


bool CardSession::connectCard()
{
    bool ok;
    {
        QMutexLocker lock(&m_mutex);
        ok = m_transport->connectCard(m_reader);
    }
    emit stateChanged();
    return ok;
}


bool CardSession::disconnectCard()
{
    bool ok;
    {
        QMutexLocker lock(&m_mutex);
        ok = m_transport->disconnectCard();
    }
    emit stateChanged();
    return ok;
}


bool CardSession::reset()
{
    bool ok;
    {
        QMutexLocker lock(&m_mutex);
        ok = m_transport->reset();
    }
    emit stateChanged();
    return ok;
}


QByteArray CardSession::transmit(const QByteArray &command)
{
    QMutexLocker lock(&m_mutex);
    return m_transport->transmit(command);
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
