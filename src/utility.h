#ifndef UTILITY_H
#define UTILITY_H

#include <QObject>


class Utility : public QObject
{
    Q_OBJECT
public:
    explicit Utility() {};

    static QByteArray hexStringToByteArray(const QString& hexString);

signals:
};

#endif
