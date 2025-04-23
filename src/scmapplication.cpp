#include "scmapplication.h"

ScmApplication::ScmApplication(int &argc, char **argv)
    : QApplication(argc, argv)
{
    pcsc = new PcscWrapper();

    cardReader = QString();
}
