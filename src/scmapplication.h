#ifndef SCMAPPLICATION_H
#define SCMAPPLICATION_H

#include <QApplication>

#include "pcscwrapper.h"


class ScmApplication : public QApplication
{
    Q_OBJECT

public:
    explicit ScmApplication(int &argc, char **argv);

    PcscWrapper *pcsc;
    QString cardReader;
};

#endif
