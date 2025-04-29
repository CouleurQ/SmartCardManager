#include "tabhandlerrunscript.h"
#include "global.h"
#include "utility.h"
#include "commands/viewsendbytes.h"
#include "commands/viewapdu.h"
#include "commands/viewterminal.h"

#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QMessageBox>
#include <QMenu>



TabHandlerRunScript::TabHandlerRunScript(QWidget *tabWidget, QWidget *parent)
    : QWidget{parent}
{
    app = qApp ? qobject_cast<ScmApplication*>(qApp) : nullptr;

    // load the widgets
    assert((stackedWidget = tabWidget->findChild<QStackedWidget *>("runScriptStackedWidget")) != nullptr);
    assert((buttonAdd = tabWidget->findChild<QPushButton *>("runScriptButtonAdd")) != nullptr);

    // Add actions to the menu
    QMenu *menu = new QMenu(this);
    QAction *action1 = menu->addAction("Send command bytes");
    action1->setData(1);
    QAction *action2 = menu->addAction("Send APDU");
    action2->setData(2);
    QAction *action3 = menu->addAction("Terminal command");
    action3->setData(3);
    buttonAdd->setMenu(menu);

    // Connect the actions to slots
    connect(action1, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(action2, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(action3, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);

}


TabHandlerRunScript::~TabHandlerRunScript()
{
}


void TabHandlerRunScript::replaceCurrentView(QWidget*view) {
    int currentIndex = stackedWidget->currentIndex();
    if (currentIndex != -1) {
        QWidget *currentView = stackedWidget->widget(currentIndex);
        stackedWidget->removeWidget(currentView);
        delete currentView;
    }

    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stackedWidget->addWidget(view);
    stackedWidget->setCurrentIndex(stackedWidget->indexOf(view));
}


void TabHandlerRunScript::buttonActionAdd()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == NULL)
        return;

    switch (action->data().toInt()) {
    case 1:
        replaceCurrentView(new ViewSendBytes());
        break;
    case 2:
        replaceCurrentView(new ViewApdu());
        break;
    case 3:
        replaceCurrentView(new ViewTerminal());
        break;
    }
}
