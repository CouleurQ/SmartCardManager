#ifndef PCSCWRAPPER_H
#define PCSCWRAPPER_H

#include <QObject>

#include "pcsclite.h"

#include "global.h"


class PcscWrapper : public QObject
{
    Q_OBJECT

public:
    explicit PcscWrapper(QObject* parent = nullptr);
    ~PcscWrapper() override;

    QStringList getReaders();
    bool connect(const QString &reader);
    bool disconnect(const QString &reader);
    bool reset(const QString &reader);
    bool isConnected(const QString &reader);
    QByteArray send(const QString &reader, const QByteArray command);

private:
    SCARDCONTEXT hContext = 0;
    SCARDHANDLE hCard = 0;
    DWORD dwActiveProtocol = 0;

private:
    SCARDCONTEXT getContext();

signals:
    void log(LogType type, const QString& hexData);
};

#endif
