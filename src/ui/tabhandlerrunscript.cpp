#include "tabhandlerrunscript.h"
#include "commands/view.h"
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
#include <QItemSelectionModel>



TabHandlerRunScript::TabHandlerRunScript(QWidget *tabWidget, QWidget *parent)
    : QWidget{parent}
{
    app = qApp ? qobject_cast<ScmApplication*>(qApp) : nullptr;

    // load the widgets
    assert((stackedWidget = tabWidget->findChild<QStackedWidget *>("runScriptStackedWidget")) != nullptr);
    assert((buttonAdd = tabWidget->findChild<QPushButton *>("runScriptButtonAdd")) != nullptr);
    assert((listView = tabWidget->findChild<QListView *>("runScriptListView")) != nullptr);

    // Add actions to the menu
    QMenu *menu = new QMenu(this);
    QAction *action1 = menu->addAction("Send command bytes");
    action1->setData(1);
    QAction *action2 = menu->addAction("Send APDU");
    action2->setData(2);
    QAction *action3 = menu->addAction("Terminal command");
    action3->setData(3);
    buttonAdd->setMenu(menu);

    // add the model to the listview
    QStandardItemModel *model = new QStandardItemModel(this);
    listView->setModel(model);
    listView->setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect the actions to slots
    connect(action1, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(action2, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(action3, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
}


TabHandlerRunScript::~TabHandlerRunScript()
{
}


void TabHandlerRunScript::replaceCurrentView(View *view) {
    int currentIndex = stackedWidget->currentIndex();
    if (currentIndex != -1) {
        QWidget *currentView = stackedWidget->widget(currentIndex);
        stackedWidget->removeWidget(currentView);
        delete currentView;
    }

    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stackedWidget->addWidget(view);
    stackedWidget->setCurrentIndex(stackedWidget->indexOf(view));

    // Connect slots
    connect(view, &View::notifyUpdateCommand, this, &TabHandlerRunScript::receiveUpdateCommand);

    // dummy call to update the data in the listview through emit -> slot callback
    view->sendUpdateCommand();
}


void TabHandlerRunScript::buttonActionAdd()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == NULL)
        return;

    // add a new item to the listview
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());

    // add item
    QStandardItem *item = new QStandardItem("");
    //item->setData(NULL, Qt::UserRole);
    model->appendRow(item);

    // select item
    QModelIndex index = model->index(model->rowCount() - 1, 0);
    listView->setCurrentIndex(index);
    listView->scrollTo(index);
    listView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);

    // set the right view
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


void TabHandlerRunScript::receiveUpdateCommand(QSharedPointer<Command> obj)
{
    QModelIndex currentIndex = listView->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->itemFromIndex(currentIndex);
        if (item) {
            item->setText(obj->commandName);
            item->setData(QVariant::fromValue(obj), Qt::UserRole);
        }
    }


    // switch(obj->type()) {
    // case Command::ObjectType::BaseType: qDebug() << "BaseType"; break;
    // case Command::ObjectType::SendBytesType: qDebug() << "SendBytesType"; break;
    // case Command::ObjectType::ApduType: qDebug() << "ApduType"; break;
    // case Command::ObjectType::TerminalType: qDebug() << "TerminalType"; break;
    // }
}
