# Unit tests (functional + UI). Build/run with: ./make.sh test
QT += core gui widgets network svgwidgets testlib
CONFIG += c++17 testcase no_testcase_installs
TEMPLATE = app
TARGET = tst_smartcardmanager

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000

CONFIG += link_pkgconfig
PKGCONFIG += libpcsclite
INCLUDEPATH += $$PWD/../src $$PWD/../src/controls

# Application sources under test (everything except main.cpp).
# Keep in sync with ../SmartCardManager.pro (the lists are intentionally not shared).
APP = $$PWD/../src
SOURCES += \
    $$APP/card/atrparser.cpp \
    $$APP/card/cardsession.cpp \
    $$APP/card/pcsctransport.cpp \
    $$APP/commands/command.cpp \
    $$APP/commands/commandapdu.cpp \
    $$APP/commands/commandfactory.cpp \
    $$APP/commands/commandsendbytes.cpp \
    $$APP/commands/commandterminal.cpp \
    $$APP/commands/commandview.cpp \
    $$APP/commands/commandviewapdu.cpp \
    $$APP/commands/commandviewsendbytes.cpp \
    $$APP/commands/commandviewterminal.cpp \
    $$APP/commands/scriptrunner.cpp \
    $$APP/controls/hexedit.cpp \
    $$APP/io/commandlistfile.cpp \
    $$APP/io/scriptfile.cpp \
    $$APP/ui/aboutdialog.cpp \
    $$APP/ui/atrview.cpp \
    $$APP/card/gpcoding.cpp \
    $$APP/card/gpdemodata.cpp \
    $$APP/ui/gpauthdialog.cpp \
    $$APP/ui/gpsendpanel.cpp \
    $$APP/ui/javacardview.cpp \
    $$APP/ui/listreorderhelper.cpp \
    $$APP/ui/logview.cpp \
    $$APP/ui/mainwindow.cpp \
    $$APP/ui/tabhandlerrunscript.cpp \
    $$APP/ui/tabhandlersendcommand.cpp \
    $$APP/utility.cpp

HEADERS += \
    $$APP/card/atrparser.h \
    $$APP/card/cardsession.h \
    $$APP/card/icardtransport.h \
    $$APP/card/pcsctransport.h \
    $$APP/commands/command.h \
    $$APP/commands/commandapdu.h \
    $$APP/commands/commandfactory.h \
    $$APP/commands/commandsendbytes.h \
    $$APP/commands/commandterminal.h \
    $$APP/commands/commandview.h \
    $$APP/commands/commandviewapdu.h \
    $$APP/commands/commandviewsendbytes.h \
    $$APP/commands/commandviewterminal.h \
    $$APP/commands/runscriptmodel.h \
    $$APP/commands/scriptrunner.h \
    $$APP/controls/hexedit.h \
    $$APP/global.h \
    $$APP/io/commandlistfile.h \
    $$APP/io/jsonwriter.h \
    $$APP/io/scriptfile.h \
    $$APP/ui/aboutdialog.h \
    $$APP/ui/atrview.h \
    $$APP/card/gpauthparams.h \
    $$APP/card/gpcoding.h \
    $$APP/card/gpdemodata.h \
    $$APP/card/gpregistry.h \
    $$APP/ui/gpauthdialog.h \
    $$APP/ui/gpsendpanel.h \
    $$APP/ui/javacardview.h \
    $$APP/ui/listreorderhelper.h \
    $$APP/ui/logview.h \
    $$APP/ui/mainwindow.h \
    $$APP/ui/tabhandlerrunscript.h \
    $$APP/ui/tabhandlersendcommand.h \
    $$APP/utility.h \
    mocktransport.h \
    testregistry.h \
    testutil.h \
    privateaccess.h

FORMS += \
    $$PWD/../res/forms/mainwindow.ui \
    $$PWD/../res/forms/aboutdialog.ui \
    $$PWD/../res/forms/commandviewapdu.ui \
    $$PWD/../res/forms/commandviewsendbytes.ui \
    $$PWD/../res/forms/commandviewterminal.ui

DEFINES += TEST_RES_DIR=\\\"$$PWD/../res\\\"

RESOURCES += $$PWD/../res/resources.qrc

SOURCES += \
    main.cpp \
    tst_utility.cpp \
    tst_atr.cpp \
    tst_javacard.cpp \
    tst_commands.cpp \
    tst_hexedit.cpp \
    tst_views.cpp \
    tst_logview.cpp \
    tst_pcsctransport.cpp \
    tst_cardsession.cpp \
    tst_scriptfile.cpp \
    tst_sendcommand.cpp \
    tst_runscript.cpp \
    tst_mainwindow.cpp
