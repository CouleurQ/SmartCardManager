#ifndef PCSCWRAPPER_H
#define PCSCWRAPPER_H

#include <QObject>

#include "pcsclite.h"

#include "global.h"


class PcscWrapper : public QObject
{
    Q_OBJECT

public:
    PcscWrapper(QObject* parent = nullptr);

    QStringList getReaders();
    bool connect(const QString &reader);
    bool disconnect(const QString &reader);
    bool reset(const QString &reader);
    bool isConnected(const QString &reader);
    QByteArray send(const QString &reader, const QByteArray command);

private:
    SCARDHANDLE hCard = 0;
    DWORD dwActiveProtocol = 0;

private:
    SCARDCONTEXT getContext();

signals:
    void log(LogType type, const QString& hexData);
};

#endif
