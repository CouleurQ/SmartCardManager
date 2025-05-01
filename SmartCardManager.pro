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
    src/commands/view.cpp \
    src/commands/viewapdu.cpp \
    src/commands/viewsendbytes.cpp \
    src/commands/viewterminal.cpp \
    src/main.cpp \
    src/scmapplication.cpp \
    src/ui/mainwindow.cpp \
    src/ui/logview.cpp \
    src/ui/aboutdialog.cpp \
    src/ui/tabhandlerrunscript.cpp \
    src/pcscwrapper.cpp \
    src/ui/tabhandlersendcommand.cpp \
    src/utility.cpp

HEADERS += \
    src/commands/command.h \
    src/commands/commandapdu.h \
    src/commands/commandsendbytes.h \
    src/commands/commandterminal.h \
    src/commands/model.h \
    src/commands/view.h \
    src/commands/viewapdu.h \
    src/commands/viewsendbytes.h \
    src/commands/viewterminal.h \
    src/global.h \
    src/scmapplication.h \
    src/controls/hexedit.h \
    src/ui/mainwindow.h \
    src/ui/logview.h \
    src/ui/aboutdialog.h \
    src/ui/tabhandlerrunscript.h \
    src/pcscwrapper.h \
    src/ui/tabhandlersendcommand.h \
    src/utility.h

FORMS += \
    res/forms/mainwindow.ui \
    res/forms/aboutdialog.ui \
    res/forms/viewapdu.ui \
    res/forms/viewsendbytes.ui \
    res/forms/viewterminal.ui

RESOURCES += \
    res/resources.qrc

DISTFILES += \
    res/commandListScheme.json \
    res/defaultCommandList.json \
    res/defaultScriptFile.json
