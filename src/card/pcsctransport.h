#ifndef PCSCTRANSPORT_H
#define PCSCTRANSPORT_H

#include "icardtransport.h"

#include "pcsclite.h"


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
    bool isConnected() const override;
    QByteArray transmit(const QByteArray &command) override;

private:
    SCARDCONTEXT getContext();

    SCARDCONTEXT hContext = 0;
    SCARDHANDLE hCard = 0;
    DWORD dwActiveProtocol = 0;
};

#endif
