QT       += core gui widgets network svgwidgets

CONFIG += c++17 link_pkgconfig
PKGCONFIG += libpcsclite

# disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/src/controls

SOURCES += \
    src/main.cpp \
    src/card/cardsession.cpp \
    src/card/pcsctransport.cpp \
    src/commands/command.cpp \
    src/commands/commandapdu.cpp \
    src/commands/commandfactory.cpp \
    src/commands/commandsendbytes.cpp \
    src/commands/commandterminal.cpp \
    src/commands/commandview.cpp \
    src/commands/commandviewapdu.cpp \
    src/commands/commandviewsendbytes.cpp \
    src/commands/commandviewterminal.cpp \
    src/controls/hexedit.cpp \
    src/io/commandlistfile.cpp \
    src/io/scriptfile.cpp \
    src/ui/aboutdialog.cpp \
    src/ui/logview.cpp \
    src/ui/mainwindow.cpp \
    src/ui/tabhandlerrunscript.cpp \
    src/ui/tabhandlersendcommand.cpp \
    src/utility.cpp

HEADERS += \
    src/card/cardsession.h \
    src/card/icardtransport.h \
    src/card/pcsctransport.h \
    src/commands/command.h \
    src/commands/commandapdu.h \
    src/commands/commandfactory.h \
    src/commands/commandsendbytes.h \
    src/commands/commandterminal.h \
    src/commands/commandview.h \
    src/commands/commandviewapdu.h \
    src/commands/commandviewsendbytes.h \
    src/commands/commandviewterminal.h \
    src/commands/runscriptmodel.h \
    src/controls/hexedit.h \
    src/global.h \
    src/io/commandlistfile.h \
    src/io/jsonwriter.h \
    src/io/scriptfile.h \
    src/ui/aboutdialog.h \
    src/ui/logview.h \
    src/ui/mainwindow.h \
    src/ui/tabhandlerrunscript.h \
    src/ui/tabhandlersendcommand.h \
    src/utility.h

FORMS += \
    res/forms/mainwindow.ui \
    res/forms/aboutdialog.ui \
    res/forms/commandviewapdu.ui \
    res/forms/commandviewsendbytes.ui \
    res/forms/commandviewterminal.ui

RESOURCES += \
    res/resources.qrc

DISTFILES += \
    res/commandListScheme.json \
    res/scriptFileScheme.json \
    res/defaultCommandList.json \
    res/defaultScriptFile.json
