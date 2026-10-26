# Unit tests (functional + UI). Build/run with: ./make.sh test
QT += core gui widgets network svgwidgets testlib
CONFIG += c++17 testcase no_testcase_installs
TEMPLATE = app
TARGET = tst_smartcardmanager

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

LIBS += -lpcsclite
INCLUDEPATH += /usr/include/PCSC
INCLUDEPATH += $$PWD/../src $$PWD/../src/controls

# Application sources under test (everything except main.cpp).
# Keep in sync with ../SmartCardManager.pro (see docs/CODE_REVIEW.md: extract a shared .pri).
APP = $$PWD/../src
SOURCES += \
    $$APP/commands/view.cpp \
    $$APP/commands/viewapdu.cpp \
    $$APP/commands/viewsendbytes.cpp \
    $$APP/commands/viewterminal.cpp \
    $$APP/scmapplication.cpp \
    $$APP/ui/mainwindow.cpp \
    $$APP/ui/logview.cpp \
    $$APP/ui/aboutdialog.cpp \
    $$APP/ui/tabhandlerrunscript.cpp \
    $$APP/ui/tabhandlersendcommand.cpp \
    $$APP/pcscwrapper.cpp \
    $$APP/utility.cpp

HEADERS += \
    $$APP/commands/command.h \
    $$APP/commands/commandapdu.h \
    $$APP/commands/commandsendbytes.h \
    $$APP/commands/commandterminal.h \
    $$APP/commands/model.h \
    $$APP/commands/view.h \
    $$APP/commands/viewapdu.h \
    $$APP/commands/viewsendbytes.h \
    $$APP/commands/viewterminal.h \
    $$APP/global.h \
    $$APP/scmapplication.h \
    $$APP/controls/hexedit.h \
    $$APP/ui/mainwindow.h \
    $$APP/ui/logview.h \
    $$APP/ui/aboutdialog.h \
    $$APP/ui/tabhandlerrunscript.h \
    $$APP/ui/tabhandlersendcommand.h \
    $$APP/pcscwrapper.h \
    $$APP/utility.h \
    testregistry.h \
    testutil.h \
    privateaccess.h

FORMS += \
    $$PWD/../res/forms/mainwindow.ui \
    $$PWD/../res/forms/aboutdialog.ui \
    $$PWD/../res/forms/viewapdu.ui \
    $$PWD/../res/forms/viewsendbytes.ui \
    $$PWD/../res/forms/viewterminal.ui

DEFINES += TEST_RES_DIR=\\\"$$PWD/../res\\\"

RESOURCES += $$PWD/../res/resources.qrc

SOURCES += \
    main.cpp \
    tst_utility.cpp \
    tst_commands.cpp \
    tst_hexedit.cpp \
    tst_views.cpp \
    tst_logview.cpp \
    tst_pcscwrapper.cpp \
    tst_sendcommand.cpp \
    tst_runscript.cpp \
    tst_mainwindow.cpp
