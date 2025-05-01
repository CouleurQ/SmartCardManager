#include "tabhandlerrunscript.h"
#include "utility.h"
#include "commands/view.h"
#include "commands/viewsendbytes.h"
#include "commands/viewapdu.h"
#include "commands/viewterminal.h"
#include "commands/commandsendbytes.h"
#include "commands/commandapdu.h"
#include "commands/commandterminal.h"
#include "commands/model.h"

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
    assert((buttonLoad = tabWidget->findChild<QPushButton *>("runScriptButtonLoad")) != nullptr);
    assert((buttonSave = tabWidget->findChild<QPushButton *>("runScriptButtonSave")) != nullptr);
    assert((stackedWidget = tabWidget->findChild<QStackedWidget *>("runScriptStackedWidget")) != nullptr);
    assert((buttonAdd = tabWidget->findChild<QPushButton *>("runScriptButtonAdd")) != nullptr);
    assert((listView = tabWidget->findChild<QListView *>("runScriptListView")) != nullptr);
    assert((lineEditFilename = tabWidget->findChild<QLineEdit *>("runScriptLineEditFilename")) != nullptr);

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
    model = new RunScriptModel(this);
    listView->setModel(model);
    listView->setContextMenuPolicy(Qt::CustomContextMenu);

    // Connect the actions to slots
    connect(buttonLoad, &QPushButton::clicked, this, &TabHandlerRunScript::buttonActionLoad);
    connect(buttonSave, &QPushButton::clicked, this, &TabHandlerRunScript::buttonActionSave);
    connect(action1, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(action2, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(action3, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(listView, &QListView::clicked, this, &TabHandlerRunScript::onItemClicked);
    connect(listView, &QListView::doubleClicked, this, &TabHandlerRunScript::onItemClicked);
    connect(listView, &QListView::customContextMenuRequested, this, &TabHandlerRunScript::showContextMenu);
    connect(listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TabHandlerRunScript::onItemSelected);
}


TabHandlerRunScript::~TabHandlerRunScript()
{
}


void TabHandlerRunScript::showContextMenu(const QPoint &pos)
{
    QModelIndex index = listView->indexAt(pos);

    QMenu menu;
    if (index.isValid()) {
        // Item right click
        menu.addAction("Delete", this, &TabHandlerRunScript::buttonActionDelete);
        menu.addAction("Duplicate", this, &TabHandlerRunScript::buttonActionDuplicate);
        menu.addSeparator();
        menu.addAction("Move up", this, &TabHandlerRunScript::buttonActionUp);
        menu.addAction("Move down", this, &TabHandlerRunScript::buttonActionDown);
    } else {
        // ListView right click
        menu.addAction("Load", this, &TabHandlerRunScript::buttonActionLoad);
        menu.addAction("Save", this, &TabHandlerRunScript::buttonActionSave);
        menu.addSeparator();
        menu.addAction("Clear commands", this, &TabHandlerRunScript::buttonActionClear);
    }

    menu.exec(listView->viewport()->mapToGlobal(pos));
}


void TabHandlerRunScript::onItemClicked(const QModelIndex &index)
{
    QSharedPointer<Command> item = model->itemFromIndex(index);

    switch(item->type()) {
    case Command::ObjectType::SendBytesType:
        replaceCurrentView(new ViewSendBytes(qSharedPointerDynamicCast<CommandSendBytes>(item)));
        break;
    case Command::ObjectType::ApduType:
        replaceCurrentView(new ViewApdu(qSharedPointerDynamicCast<CommandApdu>(item)));
        break;
    case Command::ObjectType::TerminalType:
        replaceCurrentView(new ViewTerminal(qSharedPointerDynamicCast<CommandTerminal>(item)));
        break;
    case Command::ObjectType::BaseType:
    default:
        break;
    }
}


void TabHandlerRunScript::onItemSelected(const QModelIndex &current, const QModelIndex &previous)
{
    (void) previous;

    if (current.isValid())
        onItemClicked(current);
}


void TabHandlerRunScript::replaceCurrentView(View *view) {
    int currentIndex = stackedWidget->currentIndex();
    if (currentIndex != -1) {
        QWidget *currentView = stackedWidget->widget(currentIndex);
        stackedWidget->removeWidget(currentView);
        disconnect(view);
        delete currentView;
    }

    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stackedWidget->addWidget(view);
    stackedWidget->setCurrentIndex(stackedWidget->indexOf(view));

    // Connect slots
    connect(view, &View::notifyUpdateCommand, this, &TabHandlerRunScript::receiveUpdateCommand);
}


void TabHandlerRunScript::buttonActionDelete()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid())
        model->removeRow(currentIndex.row());
}


void TabHandlerRunScript::buttonActionDuplicate()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        QSharedPointer<Command> item = model->itemFromIndex(currentIndex);


        switch(item->type()) {
        case Command::ObjectType::SendBytesType: {
            QSharedPointer<CommandSendBytes> newItem = QSharedPointer<CommandSendBytes>::create(CommandSendBytes(*qSharedPointerDynamicCast<CommandSendBytes>(item)));
            newItem->commandName.append("_copy");
            model->addCommand(newItem);
            replaceCurrentView(new ViewSendBytes(qSharedPointerDynamicCast<CommandSendBytes>(newItem)));
            break;
        }
        case Command::ObjectType::ApduType: {
            QSharedPointer<CommandApdu> newItem = QSharedPointer<CommandApdu>::create(CommandApdu(*qSharedPointerDynamicCast<CommandApdu>(item)));
            newItem->commandName.append("_copy");
            model->addCommand(newItem);
            replaceCurrentView(new ViewApdu(qSharedPointerDynamicCast<CommandApdu>(item)));
            break;
        }
        case Command::ObjectType::TerminalType: {
            QSharedPointer<CommandTerminal> newItem = QSharedPointer<CommandTerminal>::create(CommandTerminal(*qSharedPointerDynamicCast<CommandTerminal>(item)));
            newItem->commandName.append("_copy");
            model->addCommand(newItem);
            replaceCurrentView(new ViewTerminal(qSharedPointerDynamicCast<CommandTerminal>(item)));
            break;
        }
        case Command::ObjectType::BaseType:
        default:
            break;
        }

        // select item
        QModelIndex index = model->index(model->rowCount() - 1, 0);
        listView->setCurrentIndex(index);
        listView->scrollTo(index);
        listView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    }
}


void TabHandlerRunScript::buttonActionUp()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        int row = currentIndex.row();

        if (row > 0) {
            // Swap the rows (move item up by 1)
            model->insertRow(row - 1, model->takeRow(row));
            listView->setCurrentIndex(model->index(row - 1, 0));
        }
    }
}


void TabHandlerRunScript::buttonActionDown()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        int row = currentIndex.row();

        if (row < model->rowCount() - 1) {
            // Swap the rows (move item down by 1)
            model->insertRow(row + 1, model->takeRow(row));
            listView->setCurrentIndex(model->index(row + 1, 0));
        }
    }
}


void TabHandlerRunScript::buttonActionAdd()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == NULL)
        return;

    // add a new item to the listview
    switch (action->data().toInt()) {
    case 1:
        model->addCommand(QSharedPointer<CommandSendBytes>::create(CommandSendBytes()));
        replaceCurrentView(new ViewSendBytes());
        break;
    case 2:
        model->addCommand(QSharedPointer<CommandApdu>::create(CommandApdu()));
        replaceCurrentView(new ViewApdu());
        break;
    case 3:
        model->addCommand(QSharedPointer<CommandTerminal>::create(CommandTerminal()));
        replaceCurrentView(new ViewTerminal());
        break;
    }

    // select item
    QModelIndex index = model->index(model->rowCount() - 1, 0);
    listView->setCurrentIndex(index);
    listView->scrollTo(index);
    listView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
}


void TabHandlerRunScript::receiveUpdateCommand(QSharedPointer<Command> obj)
{
    QModelIndex currentIndex = listView->currentIndex();

    if (currentIndex.isValid())
        model->setData(currentIndex, QVariant::fromValue(obj), Qt::UserRole);
}


void TabHandlerRunScript::buttonActionClear()
{
    model->clear();
    listView->clearSelection();
}


void TabHandlerRunScript::buttonActionLoad()
{
    // Select a JSON file
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Select SendApdu JSON File", "", "JSON Files (*.json);;All Files (*)");

    if (filePath.isEmpty())
        return;

    _buttonActionLoad(filePath);
}


void TabHandlerRunScript::_buttonActionLoad(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit logString(LogType::Error, QString("%1: %2").arg("Failed to open file", file.errorString()));
        return;
    }


    // Read and parse the JSON data into a QJsonDocument
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        qWarning() << "Failed to parse script file";
        return;
    }

    try {
        QJsonObject jsonObject = doc.object();

        // Access and print top-level elements
        QString version = jsonObject["version"].toString();
        if (version != "1.0")
            throw std::runtime_error(QString("Invalid script file version: " + version).toStdString());

        // file seems valid - remove the old entries
        buttonActionClear();

        // Now handle the apduList, which is an array
        QJsonArray items = jsonObject["items"].toArray();
        for (const QJsonValue &value : items) {
            QJsonObject item = value.toObject();
            QString name = item["name"].toString();
            QString type = item["type"].toString();
            if (type == "SendBytes") {
                qDebug() << "Adding SENDBYTES " + name;
                QSharedPointer<CommandSendBytes> command = QSharedPointer<CommandSendBytes>::create(CommandSendBytes());
                command->commandName = name;
                command->commandData = Utility::hexStringToByteArray(item["cmd"].toString());
                command->expectedResponse = Utility::hexStringToByteArray(item["expectedResponse"].toString());
                model->addCommand(command);
                replaceCurrentView(new ViewSendBytes(command));
            } else if (type == "SendAPDU") {
                qDebug() << "Adding SENDAPDU " + name;
                QSharedPointer<CommandApdu> command = QSharedPointer<CommandApdu>::create(CommandApdu());
                command->commandName = name;
                command->cla = Utility::hexStringToByteArray(item["cla"].toString());
                command->ins = Utility::hexStringToByteArray(item["ins"].toString());
                command->p1 = Utility::hexStringToByteArray(item["p1"].toString());
                command->p2 = Utility::hexStringToByteArray(item["p2"].toString());
                command->commandData = Utility::hexStringToByteArray(item["data"].toString());
                command->le = Utility::hexStringToByteArray(item["le"].toString());
                command->expectedResponse = Utility::hexStringToByteArray(item["expectedResponse"].toString());
                model->addCommand(command);
                replaceCurrentView(new ViewApdu(command));
            } else if (type == "TerminalCmd") {
                qDebug() << "Adding TERMINALCMD " + name;
                QSharedPointer<CommandTerminal> command = QSharedPointer<CommandTerminal>::create(CommandTerminal());
                command->commandName = name;
                model->addCommand(command);
                replaceCurrentView(new ViewTerminal(command));
            }
        }

        // all seems good - set the filename
        lineEditFilename->setText(filePath);

        // focus the first item
        QModelIndex index = model->index(0, 0);
        listView->setCurrentIndex(index);
        listView->scrollTo(index);
        listView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
    } catch (const std::exception e) {
        emit logString(LogType::Error, e.what());
    }
}


void TabHandlerRunScript::buttonActionSave()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save File", "", "JSON Files (*.json);;All Files (*)");

    if (filePath.isEmpty())
        return;

    if (!filePath.endsWith(".json", Qt::CaseInsensitive)) {
        filePath.append(".json");
    }

    try {
        // we serialize manually to keep the oder for better human editing
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            emit logString(LogType::Error, QString("%1: %2").arg("Cannot write to file", file.errorString()));
            return;
        }

        QTextStream out(&file);
        out << "{\n";
        out << "  \"version\": \"1.0\",\n";
        out << "  \"dateTime\": \"" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\",\n";
        out << "  \"id\": \"" << QUuid::createUuid().toString().remove('{').remove('}') << "\",\n";
        out << "  \"items\": [\n";

        for (int row = 0; row < listView->model()->rowCount(); ++row) {
            QSharedPointer<Command> item = model->item(row);

            out << "    {\n";
            out << "      \"name\": \"" << item->commandName << "\",\n";
            out << "      \"seq\": \"" << row << "\",\n";

            switch(item->type()) {
            case Command::ObjectType::SendBytesType: {
                QSharedPointer<CommandSendBytes> specificItem = qSharedPointerDynamicCast<CommandSendBytes>(item);
                out << "      \"type\": \"SendBytes\",\n";
                out << "      \"cmd\": \"" << QString(specificItem->commandData.toHex(' ').toUpper()) << "\",\n";
                out << "      \"expectedResponse\": \"" << QString(specificItem->expectedResponse.toHex(' ').toUpper()) << "\"\n";
                break;
            }
            case Command::ObjectType::ApduType: {
                QSharedPointer<CommandApdu> specificItem = qSharedPointerDynamicCast<CommandApdu>(item);
                out << "      \"type\": \"SendAPDU\",\n";
                out << "      \"cla\": \"" << QString(specificItem->cla.toHex(' ').toUpper()) << "\",\n";
                out << "      \"ins\": \"" << QString(specificItem->ins.toHex(' ').toUpper()) << "\",\n";
                out << "      \"p1\": \"" << QString(specificItem->p1.toHex(' ').toUpper()) << "\",\n";
                out << "      \"p2\": \"" << QString(specificItem->p2.toHex(' ').toUpper()) << "\",\n";
                out << "      \"cmd\": \"" << QString(specificItem->commandData.toHex(' ').toUpper()) << "\",\n";
                out << "      \"le\": \"" << QString(specificItem->le.toHex(' ').toUpper()) << "\",\n";
                out << "      \"expectedResponse\": \"" << QString(specificItem->expectedResponse.toHex(' ').toUpper()) << "\"\n";
                break;
            }
            case Command::ObjectType::TerminalType: {
                QSharedPointer<CommandTerminal> specificItem = qSharedPointerDynamicCast<CommandTerminal>(item);
                out << "      \"type\": \"TerminalCmd\"\n";
                break;
            }
            case Command::ObjectType::BaseType:
            default:
                break;
            }

            if (row <= listView->model()->rowCount() - 2)
                out << "    },\n";
            else
                out << "    }\n";
        }
        out << "  ]\n";
        out << "}\n";
        file.close();

        QMessageBox::information(this, "Success", "File saved successfully.");
    } catch (std::exception e) {
        emit logString(LogType::Error, e.what());
    }
}
