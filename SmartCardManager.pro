QT       += core gui network svgwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

LIBS += -lpcsclite
INCLUDEPATH += /usr/include/PCSC

INCLUDEPATH += $$PWD/src/controls

SOURCES += \
    src/aboutdialog.cpp \
    src/logview.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/pcscwrapper.cpp

HEADERS += \
    src/aboutdialog.h \
    src/controls/hexedit.h \
    src/global.h \
    src/hexedit.h \
    src/logview.h \
    src/mainwindow.h \
    src/pcscwrapper.h

FORMS += \
    res/forms/aboutdialog.ui \
    res/forms/mainwindow.ui

RESOURCES += \
    res/resources.qrc
