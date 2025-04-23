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

    connect(buttonLoad, &QPushButton::clicked, this, &TabHandlerSendApdu::sendApduLoad);
    connect(buttonSave, &QPushButton::clicked, this, &TabHandlerSendApdu::sendApduSave);
    connect(buttonSend, &QPushButton::clicked, this, &TabHandlerSendApdu::sendApduSend);
    connect(listView, &QListView::clicked, this, &TabHandlerSendApdu::onItemClicked);
    connect(listView, &QListView::doubleClicked, this, &TabHandlerSendApdu::onItemDoubleClicked);
    connect(lineEditCommandName, &QLineEdit::textChanged, this, &TabHandlerSendApdu::updateCommandName);
    //connect(hexEditCommandApdu, &HexEdit::textChanged, this, &TabHandlerSendApdu::updateCommandApdu);
    connect(listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &TabHandlerSendApdu::onItemSelected);
}


TabHandlerSendApdu::~TabHandlerSendApdu()
{
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


void TabHandlerSendApdu::updateCommandApdu(const QString &newText)
{
    if (newText.length() == 0)
        return;

    QModelIndex currentIndex = listView->currentIndex();

    if (currentIndex.isValid()) {
        QStandardItem *item = qobject_cast<QStandardItemModel*>(listView->model())->itemFromIndex(currentIndex);
        if (item)
            item->setText(Utility::hexStringToByteArray(newText));
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


void TabHandlerSendApdu::onItemSelected(const QItemSelection &selected, const QItemSelection &deselected)
{
    (void) deselected;

    if (!selected.isEmpty()) {
        QModelIndex selectedIndex = selected.indexes().first();

        if (selectedIndex.isValid())
            onItemClicked(selectedIndex);
    }
}


void TabHandlerSendApdu::onItemDoubleClicked(const QModelIndex &index)
{
    onItemClicked(index);
    sendApduSend();
}


void TabHandlerSendApdu::sendApduLoad()
{
    // Select a JSON file
    QString filePath = QFileDialog::getOpenFileName(nullptr, "Select SendApdu JSON File", "", "JSON Files (*.json);;All Files (*)");

    if (filePath.isEmpty())
        return;

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


void TabHandlerSendApdu::sendApduSave()
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

        QMessageBox::critical(this, "Success", "File saved successfully.");
    } catch (std::exception e) {
        emit logString(LogType::Error, e.what());
    }
}


void TabHandlerSendApdu::sendApduSend()
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


