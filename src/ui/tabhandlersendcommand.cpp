#include "tabhandlersendcommand.h"
#include "global.h"
#include "utility.h"

#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QMessageBox>
#include <QMenu>
#include <QDateTime>
#include <QUuid>
#include <QTextStream>
#include <QMimeData>
#include <QDropEvent>



TabHandlerSendCommand::TabHandlerSendCommand(QWidget *tabWidget, QObject *parent)
    : QObject{parent}
    , tabWidget(tabWidget)
{
    app = qApp ? qobject_cast<ScmApplication*>(qApp) : nullptr;

    buttonLoad = tabWidget->findChild<QPushButton *>("sendCommandButtonLoad");
    Q_ASSERT(buttonLoad != nullptr);
    buttonSave = tabWidget->findChild<QPushButton *>("sendCommandButtonSave");
    Q_ASSERT(buttonSave != nullptr);
    buttonSend = tabWidget->findChild<QPushButton *>("sendCommandButtonSend");
    Q_ASSERT(buttonSend != nullptr);
    lineEditFilename = tabWidget->findChild<QLineEdit *>("sendCommandLineEditFilename");
    Q_ASSERT(lineEditFilename != nullptr);
    listView = tabWidget->findChild<QListView *>("sendCommandListView");
    Q_ASSERT(listView != nullptr);
    lineEditCommandName = tabWidget->findChild<QLineEdit *>("sendCommandLineEditCommandName");
    Q_ASSERT(lineEditCommandName != nullptr);
    hexEditCommandApdu = tabWidget->findChild<HexEdit *>("sendCommandHexEditCommandApdu");
    Q_ASSERT(hexEditCommandApdu != nullptr);

    QStandardItemModel *model = new QStandardItemModel(this);
    listView->setModel(model);
    listView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(buttonLoad, &QPushButton::clicked, this, &TabHandlerSendCommand::buttonActionLoad);
    connect(buttonSave, &QPushButton::clicked, this, &TabHandlerSendCommand::buttonActionSave);
    connect(buttonSend, &QPushButton::clicked, this, &TabHandlerSendCommand::buttonActionSend);
    connect(listView, &QListView::clicked, this, &TabHandlerSendCommand::onItemClicked);
    connect(listView, &QListView::doubleClicked, this, &TabHandlerSendCommand::onItemDoubleClicked);
    connect(listView, &QListView::customContextMenuRequested, this, &TabHandlerSendCommand::showContextMenu);
    connect(listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TabHandlerSendCommand::onItemSelected);
    connect(lineEditCommandName, &QLineEdit::textChanged, this, &TabHandlerSendCommand::updateCommandName);
    connect(hexEditCommandApdu, &HexEdit::textChanged, this, &TabHandlerSendCommand::updateCommandApdu);

    listView->installEventFilter(this);

    // drag events are delivered to the viewport of the item view
    listView->viewport()->setAcceptDrops(true);
    listView->viewport()->installEventFilter(this);
}


TabHandlerSendCommand::~TabHandlerSendCommand()
{
}


void TabHandlerSendCommand::showContextMenu(const QPoint &pos)
{
    QModelIndex index = listView->indexAt(pos);

    QMenu menu(tabWidget);
    if (index.isValid()) {
        // Item right click
        menu.addAction("Delete", this, &TabHandlerSendCommand::buttonActionDelete);
        menu.addAction("Duplicate", this, &TabHandlerSendCommand::buttonActionDuplicate);
        menu.addSeparator();
        menu.addAction("Move up", this, &TabHandlerSendCommand::buttonActionUp);
        menu.addAction("Move down", this, &TabHandlerSendCommand::buttonActionDown);
    } else {
        // ListView right click
        menu.addAction("Add command", this, &TabHandlerSendCommand::buttonActionAdd);
        menu.addSeparator();
        menu.addAction("Load", this, &TabHandlerSendCommand::buttonActionLoad);
        menu.addAction("Save", this, &TabHandlerSendCommand::buttonActionSave);
        menu.addSeparator();
        menu.addAction("Clear commands", this, &TabHandlerSendCommand::buttonActionClear);
    }

    menu.exec(listView->viewport()->mapToGlobal(pos));
}


void TabHandlerSendCommand::updateCommandName(const QString &newText)
{
    QModelIndex currentIndex = listView->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->itemFromIndex(currentIndex);
        if (item)
            item->setText(newText);
    }
}


void TabHandlerSendCommand::updateCommandApdu()
{
    QString newText = hexEditCommandApdu->toPlainText();

    QModelIndex currentIndex = listView->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->itemFromIndex(currentIndex);
        if (item)
            item->setData(Utility::hexStringToByteArray(newText), Qt::UserRole);
    }
}


void TabHandlerSendCommand::onItemClicked(const QModelIndex &index)
{
    QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->itemFromIndex(index);

    if (item) {
        lineEditCommandName->setText(item->text());
        hexEditCommandApdu->setText(item->data(Qt::UserRole).toByteArray().toHex(' ').toUpper());
    }
}


void TabHandlerSendCommand::onItemSelected(const QModelIndex &current, const QModelIndex &previous)
{
    (void) previous;

    if (current.isValid()) {
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
        if (model) {
            QStandardItem *item = model->itemFromIndex(current);
            if (item)
                onItemClicked(item->index());
        }
    }
}


void TabHandlerSendCommand::onItemDoubleClicked(const QModelIndex &index)
{
    onItemClicked(index);
    buttonActionSend();
}


void TabHandlerSendCommand::buttonActionClear()
{
    qobject_cast<QStandardItemModel *>(listView->model())->clear();
    listView->clearSelection();
}


void TabHandlerSendCommand::buttonActionDelete()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
        if (model)
            model->removeRow(currentIndex.row());
    }
}


void TabHandlerSendCommand::buttonActionDuplicate()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
        if (model) {
            // add item
            QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->itemFromIndex(currentIndex);

            QString itemText = item->text() + "_copy";
            QByteArray itemData = item->data(Qt::UserRole).toByteArray();
            _buttonActionAdd(itemText, itemData);
        }
    }
}


void TabHandlerSendCommand::buttonActionUp()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
        if (model) {
            int row = currentIndex.row();

            if (row > 0) {
                // Swap the rows (move item up by 1)
                model->insertRow(row - 1, model->takeRow(row));
                listView->setCurrentIndex(model->index(row - 1, 0));
            }
        }
    }
}


void TabHandlerSendCommand::buttonActionDown()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
        if (model) {
            int row = currentIndex.row();

            if (row < model->rowCount() - 1) {
                // Swap the rows (move item down by 1)
                model->insertRow(row + 1, model->takeRow(row));
                listView->setCurrentIndex(model->index(row + 1, 0));
            }
        }
    }
}


void TabHandlerSendCommand::buttonActionAdd()
{
    _buttonActionAdd("new command", QByteArray());
}


void TabHandlerSendCommand::_buttonActionAdd(const QString &cmdName, const QByteArray &cmdData)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());

    // add item
    QStandardItem *item = new QStandardItem(cmdName);
    item->setData(cmdData, Qt::UserRole);
    model->appendRow(item);

    // select item
    QModelIndex index = model->index(model->rowCount() - 1, 0);
    listView->setCurrentIndex(index);
    listView->scrollTo(index);
    listView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);

    lineEditCommandName->setFocus();
    lineEditCommandName->selectAll();
}


void TabHandlerSendCommand::buttonActionLoad()
{
    // Select a JSON file
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Select Command File", "", "JSON Files (*.json);;All Files (*)");

    if (filePath.isEmpty())
        return;

    _buttonActionLoad(filePath);
}


void TabHandlerSendCommand::_buttonActionLoad(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit logString(LogType::Error, QString("%1: %2").arg("Failed to open file", file.errorString()));
        return;
    }


    // Read and parse the JSON data into a QJsonDocument
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull()) {
        emit logString(LogType::Error, QString("%1: %2").arg("Failed to parse command file", parseError.errorString()));
        return;
    }

    try {
        QJsonObject jsonObject = doc.object();

        // Access and print top-level elements
        QString version = jsonObject["version"].toString();
        if (version != "1.0")
            throw std::runtime_error(QString("Invalid json version: " + version).toStdString());

        // file seems valid - remove the old entries
        buttonActionClear();

        // Now handle the apduList, which is an array
        QJsonArray apduList = jsonObject["apduList"].toArray();
        for (int i = 0; i < apduList.size(); ++i) {
            QJsonObject apduObject = apduList[i].toObject();
            QString name = apduObject["name"].toString();
            QString cmd = apduObject["cmd"].toString();
            qDebug() << "Adding " + name + " with data: " << cmd;

            QStandardItem *item = new QStandardItem(name);

            item->setData(Utility::hexStringToByteArray(cmd), Qt::UserRole);

            qobject_cast<QStandardItemModel *>(listView->model())->appendRow(item);
        }

        // all worked - set the filename
        lineEditFilename->setText(filePath);
        lineEditCommandName->setText("");
        hexEditCommandApdu->setText("");
    } catch (const std::exception &e) {
        emit logString(LogType::Error, e.what());
    }
}


void TabHandlerSendCommand::buttonActionSave()
{
    QString filePath = QFileDialog::getSaveFileName(tabWidget, "Save Command File", "", "JSON Files (*.json);;All Files (*)");

    if (filePath.isEmpty())
        return;

    if (!filePath.endsWith(".json", Qt::CaseInsensitive)) {
        filePath.append(".json");
    }

    try {
        QJsonArray apduListArray;

        for (int row = 0; row < listView->model()->rowCount(); ++row) {
            QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->item(row);

            // Extract the text and the attached data (QByteArray)
            QString itemText = item->text();
            QByteArray itemData = item->data(Qt::UserRole).toByteArray();

            // Create a JSON object for each item
            QJsonObject apduItem;
            apduItem["name"] = itemText;
            apduItem["cmd"] = QString(itemData.toHex(' ').toUpper());

            // Add the item to the JSON array
            apduListArray.append(apduItem);
        }

        QJsonObject jsonObject;
        jsonObject["version"] = "1.0";
        jsonObject["dateTime"] = QDateTime::currentDateTime().toString(Qt::ISODate); // ISO 8601
        jsonObject["id"] = QUuid::createUuid().toString().remove('{').remove('}');
        jsonObject["apduList"] = apduListArray;


        QString jsonString = QJsonDocument(jsonObject).toJson(QJsonDocument::Indented);

        // Open the file for writing
        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            emit logString(LogType::Error, QString("%1: %2").arg("Cannot write to file", file.errorString()));
            return;
        }

        QTextStream out(&file);
        out << jsonString;

        file.close();

        lineEditFilename->setText(filePath);
        emit logString(LogType::Info, QString("%1: %2").arg("Command file saved", filePath));
    } catch (const std::exception &e) {
        emit logString(LogType::Error, e.what());
    }
}


void TabHandlerSendCommand::buttonActionSend()
{
    QByteArray cmd = Utility::hexStringToByteArray(hexEditCommandApdu->toPlainText());
    if (cmd.length() == 0)
        return;

    emit logBytes(LogType::CardCmd, cmd);

    QByteArray response = app->pcsc->send(app->cardReader, cmd);
    if (response.length() >= 2)
        emit logBytes(LogType::CardRsp, response);
    else
        emit logString(LogType::Error, "No response from card");

    emit update();
}


bool TabHandlerSendCommand::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == listView || obj == listView->viewport()) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            QDropEvent *dragEvent = static_cast<QDropEvent *>(event);
            if (dragEvent->mimeData()->hasUrls()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        } else if (event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent *>(event);
            QStringList filePaths;
            for (const QUrl &url : dropEvent->mimeData()->urls()) {
                if (url.isLocalFile()) {
                    filePaths << url.toLocalFile();
                }
            }
            if (!filePaths.isEmpty())
                _buttonActionLoad(*filePaths.begin());

            dropEvent->acceptProposedAction();
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}
