QT       += core gui network svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

LIBS += -lpcsclite
INCLUDEPATH += /usr/include/PCSC

SOURCES += \
    src/aboutdialog.cpp \
    src/logview.cpp \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/aboutdialog.h \
    src/global.h \
    src/logview.h \
    src/mainwindow.h

FORMS += \
    res/forms/aboutdialog.ui \
    res/forms/mainwindow.ui

RESOURCES += \
    res/resources.qrc
