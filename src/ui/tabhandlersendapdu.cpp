#include "tabhandlersendapdu.h"
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



TabHandlerSendApdu::TabHandlerSendApdu(QWidget *tabWidget, QWidget *parent)
    : QWidget{parent}
{
    app = qApp ? qobject_cast<ScmApplication*>(qApp) : nullptr;

    assert((buttonLoad = tabWidget->findChild<QPushButton *>("sendApduButtonLoad")) != nullptr);
    assert((buttonSave = tabWidget->findChild<QPushButton *>("sendApduButtonSave")) != nullptr);
    assert((buttonSend = tabWidget->findChild<QPushButton *>("sendApduButtonSend")) != nullptr);
    assert((lineEditFilename = tabWidget->findChild<QLineEdit *>("sendApduLineEditFilename")) != nullptr);
    assert((listView = tabWidget->findChild<QListView *>("sendApduListView")) != nullptr);
    assert((lineEditCommandName = tabWidget->findChild<QLineEdit *>("sendApduLineEditCommandName")) != nullptr);
    assert((hexEditCommandApdu = tabWidget->findChild<HexEdit *>("sendApduHexEditCommandApdu")) != nullptr);

    QStandardItemModel *model = new QStandardItemModel(this);
    listView->setModel(model);
    listView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(buttonLoad, &QPushButton::clicked, this, &TabHandlerSendApdu::buttonActionLoad);
    connect(buttonSave, &QPushButton::clicked, this, &TabHandlerSendApdu::buttonActionSave);
    connect(buttonSend, &QPushButton::clicked, this, &TabHandlerSendApdu::buttonActionSend);
    connect(listView, &QListView::clicked, this, &TabHandlerSendApdu::onItemClicked);
    connect(listView, &QListView::doubleClicked, this, &TabHandlerSendApdu::onItemDoubleClicked);
    connect(listView, &QListView::customContextMenuRequested, this, &TabHandlerSendApdu::showContextMenu);
    connect(listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TabHandlerSendApdu::onItemSelected);
    connect(lineEditCommandName, &QLineEdit::textChanged, this, &TabHandlerSendApdu::updateCommandName);
    connect(hexEditCommandApdu, &HexEdit::textChanged, this, &TabHandlerSendApdu::updateCommandApdu);

    listView->installEventFilter(this);

    setAcceptDrops(true);
}


TabHandlerSendApdu::~TabHandlerSendApdu()
{
}


void TabHandlerSendApdu::showContextMenu(const QPoint &pos)
{
    QModelIndex index = listView->indexAt(pos);

    QMenu menu;
    if (index.isValid()) {
        // Item right click
        menu.addAction("Delete", this, &TabHandlerSendApdu::buttonActionDelete);
        menu.addAction("Duplicate", this, &TabHandlerSendApdu::buttonActionDuplicate);
        menu.addSeparator();
        menu.addAction("Move up", this, &TabHandlerSendApdu::buttonActionUp);
        menu.addAction("Move down", this, &TabHandlerSendApdu::buttonActionDown);
    } else {
        // ListView right click
        menu.addAction("Add command", this, &TabHandlerSendApdu::buttonActionAdd);
        menu.addSeparator();
        menu.addAction("Load", this, &TabHandlerSendApdu::buttonActionLoad);
        menu.addAction("Save", this, &TabHandlerSendApdu::buttonActionSave);
        menu.addSeparator();
        menu.addAction("Clear commands", this, &TabHandlerSendApdu::buttonActionClear);
    }

    menu.exec(listView->viewport()->mapToGlobal(pos));
}


void TabHandlerSendApdu::updateCommandName(const QString &newText)
{
    if (newText.length() == 0)
        return;

    QModelIndex currentIndex = listView->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->itemFromIndex(currentIndex);
        if (item)
            item->setText(newText);
    }
}


void TabHandlerSendApdu::updateCommandApdu()
{
    QString newText = hexEditCommandApdu->toPlainText();
    if (newText.length() == 0)
        return;

    QModelIndex currentIndex = listView->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->itemFromIndex(currentIndex);
        if (item)
            item->setData(Utility::hexStringToByteArray(newText), Qt::UserRole);
    }
}


void TabHandlerSendApdu::onItemClicked(const QModelIndex &index)
{
    QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->itemFromIndex(index);

    if (item) {
        lineEditCommandName->setText(item->text());
        hexEditCommandApdu->setText(item->data(Qt::UserRole).toByteArray().toHex(' ').toUpper());
    }
}


void TabHandlerSendApdu::onItemSelected(const QModelIndex &current, const QModelIndex &previous)
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


void TabHandlerSendApdu::onItemDoubleClicked(const QModelIndex &index)
{
    onItemClicked(index);
    buttonActionSend();
}


void TabHandlerSendApdu::buttonActionClear()
{
    qobject_cast<QStandardItemModel *>(listView->model())->clear();
    listView->clearSelection();
}


void TabHandlerSendApdu::buttonActionDelete()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
        if (model)
            model->removeRow(currentIndex.row());
    }
}


void TabHandlerSendApdu::buttonActionDuplicate()
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


void TabHandlerSendApdu::buttonActionUp()
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


void TabHandlerSendApdu::buttonActionDown()
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


void TabHandlerSendApdu::buttonActionAdd()
{
    _buttonActionAdd("new command", QByteArray());
}


void TabHandlerSendApdu::_buttonActionAdd(const QString &cmdName, const QByteArray &cmdData)
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


void TabHandlerSendApdu::buttonActionLoad()
{
    // Select a JSON file
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Select SendApdu JSON File", "", "JSON Files (*.json);;All Files (*)");

    if (filePath.isEmpty())
        return;

    _buttonActionLoad(filePath);
}


void TabHandlerSendApdu::_buttonActionLoad(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit logString(LogType::Error, QString("%1: %2").arg("Failed to open file", file.errorString()));
        return;
    }


    // Read and parse the JSON data into a QJsonDocument
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (doc.isNull()) {
        qWarning() << "Failed to parse JSON";
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
    } catch (const std::exception e) {
        emit logString(LogType::Error, e.what());
    }
}


void TabHandlerSendApdu::buttonActionSave()
{
    QString filePath = QFileDialog::getSaveFileName(this, "Save File", "", "JSON Files (*.json);;All Files (*)");

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

        QMessageBox::information(this, "Success", "File saved successfully.");
    } catch (std::exception e) {
        emit logString(LogType::Error, e.what());
    }
}


void TabHandlerSendApdu::buttonActionSend()
{
    QByteArray cmd = Utility::hexStringToByteArray(hexEditCommandApdu->toPlainText());
    if (cmd.length() == 0)
        return;

    emit logBytes(LogType::CardCmd, cmd);

    QByteArray response = app->pcsc->send(app->cardReader, cmd);
    if (response.length() >= 2)
        emit logBytes(LogType::CardRsp, response);

    emit update();
}


bool TabHandlerSendApdu::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == listView) {
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent *>(event);
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

    return QWidget::eventFilter(obj, event);
}
