QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000


SOURCES += \
    src/aboutdialog.cpp \
    src/main.cpp \
    src/mainwindow.cpp

HEADERS += \
    src/aboutdialog.h \
    src/mainwindow.h

FORMS += \
    res/aboutdialog.ui \
    res/mainwindow.ui
