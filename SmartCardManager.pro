QT       += core gui network svgwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

LIBS += -lpcsclite
INCLUDEPATH += /usr/include/PCSC

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/src/controls

SOURCES += \
    src/main.cpp \
    src/scmapplication.cpp \
    src/ui/mainwindow.cpp \
    src/ui/logview.cpp \
    src/ui/aboutdialog.cpp \
    src/ui/tabhandlersendapdu.cpp \
    src/pcscwrapper.cpp \
    src/utility.cpp

HEADERS += \
    src/global.h \
    src/scmapplication.h \
    src/controls/hexedit.h \
    src/ui/mainwindow.h \
    src/ui/logview.h \
    src/ui/aboutdialog.h \
    src/ui/tabhandlersendapdu.h \
    src/pcscwrapper.h \
    src/utility.h

FORMS += \
    res/forms/mainwindow.ui \
    res/forms/aboutdialog.ui

RESOURCES += \
    res/resources.qrc

DISTFILES += \
    res/apduScheme.json \
    res/defaultApduList.json
