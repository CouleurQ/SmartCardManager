#ifndef PCSCTRANSPORT_H
#define PCSCTRANSPORT_H

#include "icardtransport.h"

#include "pcscplatform.h"

#include <QMutex>


class PcscTransport : public ICardTransport
{
    Q_OBJECT

public:
    explicit PcscTransport(QObject *parent = nullptr);
    ~PcscTransport() override;

    QStringList readers() override;
    bool connectCard(const QString &reader) override;
    bool disconnectCard() override;
    bool reset() override;
    bool coldReset() override;
    bool isConnected() const override;
    QByteArray atr() const override;
    QByteArray transmit(const QByteArray &command) override;
    Presence waitPresenceChange(const QString &reader, Presence known, int timeoutMs) override;

private:
    SCARDCONTEXT getContext();
    bool reconnect(DWORD initialization, const char *what);

    SCARDCONTEXT hContext = 0;
    SCARDHANDLE hCard = 0;
    DWORD dwActiveProtocol = 0;
    QByteArray cardAtr;
    void readAtr();

    QMutex statusMutex;
    SCARDCONTEXT hStatusContext = 0;
};

#endif
