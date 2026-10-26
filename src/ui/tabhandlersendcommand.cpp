#include "tabhandlersendcommand.h"
#include "global.h"
#include "utility.h"
#include "io/commandlistfile.h"

#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItemModel>
#include "listreorderhelper.h"
#include <QPalette>
#include <QStandardItem>
#include <QMessageBox>
#include <QMenu>
#include <QDateTime>
#include <QUuid>
#include <QTextStream>
#include <QMimeData>
#include <QDropEvent>



TabHandlerSendCommand::TabHandlerSendCommand(QWidget *tabWidget, CardSession *session, QObject *parent)
    : QObject{parent}
    , session(session)
    , tabWidget(tabWidget)
{
    Q_ASSERT(session != nullptr);

    buttonLoad = tabWidget->findChild<QPushButton *>("sendCommandButtonLoad");
    Q_ASSERT(buttonLoad != nullptr);
    buttonSave = tabWidget->findChild<QPushButton *>("sendCommandButtonSave");
    Q_ASSERT(buttonSave != nullptr);
    buttonAdd = tabWidget->findChild<QPushButton *>("sendCommandButtonAdd");
    Q_ASSERT(buttonAdd != nullptr);
    lineEditSw = tabWidget->findChild<QLineEdit *>("sendCommandLineEditSw");
    Q_ASSERT(lineEditSw != nullptr);
    lineEditSwText = tabWidget->findChild<QLineEdit *>("sendCommandLineEditSwText");
    Q_ASSERT(lineEditSwText != nullptr);
    textEditResponse = tabWidget->findChild<QTextEdit *>("sendCommandTextEditResponse");
    Q_ASSERT(textEditResponse != nullptr);
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

    // read-only output fields look different from editable ones
    for (QWidget *w : {static_cast<QWidget *>(lineEditSw), static_cast<QWidget *>(lineEditSwText), static_cast<QWidget *>(textEditResponse)}) {
        QPalette pal = w->palette();
        pal.setColor(QPalette::Base, pal.color(QPalette::Window));
        w->setPalette(pal);
    }

    QStandardItemModel *model = new QStandardItemModel(this);
    listView->setModel(model);
    listView->setContextMenuPolicy(Qt::CustomContextMenu);
    reorder = new ListReorderHelper(listView, [this](const QList<int> &rows, int target) { moveItems(rows, target); });

    connect(buttonLoad, &QPushButton::clicked, this, &TabHandlerSendCommand::buttonActionLoad);
    connect(buttonSave, &QPushButton::clicked, this, &TabHandlerSendCommand::buttonActionSave);
    connect(buttonAdd, &QPushButton::clicked, this, &TabHandlerSendCommand::buttonActionAdd);
    connect(buttonSend, &QPushButton::clicked, this, &TabHandlerSendCommand::buttonActionSend);
    connect(listView, &QListView::clicked, this, &TabHandlerSendCommand::onItemClicked);
    connect(listView, &QListView::doubleClicked, this, &TabHandlerSendCommand::onItemDoubleClicked);
    connect(listView, &QListView::customContextMenuRequested, this, &TabHandlerSendCommand::showContextMenu);
    connect(listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TabHandlerSendCommand::onItemSelected);
    connect(lineEditCommandName, &QLineEdit::textChanged, this, &TabHandlerSendCommand::updateCommandName);
    connect(hexEditCommandApdu, &HexEdit::textChanged, this, &TabHandlerSendCommand::updateCommandApdu);

    connect(session, &CardSession::responseReceived, this, &TabHandlerSendCommand::onResponseReceived);

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

    clearResponse();

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
    item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
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
    QList<CommandListFile::Entry> entries;
    QString error;
    if (!CommandListFile::load(filePath, &entries, &error)) {
        emit logString(LogType::Error, error);
        return;
    }

    // file seems valid - replace the old entries
    buttonActionClear();
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
    for (const auto &entry : entries) {
        QStandardItem *item = new QStandardItem(entry.name);
        item->setFlags(item->flags() & ~Qt::ItemIsDropEnabled);
        item->setData(entry.data, Qt::UserRole);
        model->appendRow(item);
    }

    lineEditFilename->setText(filePath);
    lineEditCommandName->setText("");
    hexEditCommandApdu->setText("");
}


void TabHandlerSendCommand::buttonActionSave()
{
    QString filePath = QFileDialog::getSaveFileName(tabWidget, "Save Command File", "", "JSON Files (*.json);;All Files (*)");

    if (filePath.isEmpty())
        return;

    if (!filePath.endsWith(".json", Qt::CaseInsensitive)) {
        filePath.append(".json");
    }

    QList<CommandListFile::Entry> entries;
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
    for (int row = 0; row < model->rowCount(); ++row) {
        QStandardItem *item = model->item(row);
        entries.append({item->text(), item->data(Qt::UserRole).toByteArray()});
    }

    QString error;
    if (!CommandListFile::save(filePath, entries, &error)) {
        emit logString(LogType::Error, error);
        return;
    }

    lineEditFilename->setText(filePath);
    emit logString(LogType::Info, QString("%1: %2").arg("Command file saved", filePath));
}


void TabHandlerSendCommand::buttonActionSend()
{
    QByteArray cmd = Utility::hexStringToByteArray(hexEditCommandApdu->toPlainText());
    if (cmd.length() == 0)
        return;

    emit logBytes(LogType::CardCmd, cmd);

    sentIndex = QPersistentModelIndex(listView->currentIndex());
    sentPending = true;
    session->sendAsync(cmd);
}


void TabHandlerSendCommand::onResponseReceived(const QByteArray &command, const QByteArray &response)
{
    Q_UNUSED(command)

    // the response belongs to the command that was sent, not to whatever is selected now
    if (!sentPending || sentIndex == QPersistentModelIndex(listView->currentIndex()))
        showResponse(response);
    if (response.length() >= 2)
        emit logBytes(LogType::CardRsp, response);
    else
        emit logString(LogType::Error, "No response from card");

    emit update();
}


void TabHandlerSendCommand::moveItems(const QList<int> &rows, int target)
{
    QStandardItemModel *model = qobject_cast<QStandardItemModel *>(listView->model());
    const int current = listView->currentIndex().row();
    int newCurrent = -1;

    // take the rows out (bottom up), then put them back in front of the former target row
    QList<QList<QStandardItem *>> taken;
    for (int i = rows.size() - 1; i >= 0; --i)
        taken.prepend(model->takeRow(rows.at(i)));
    int insertAt = target;
    for (int r : rows)
        if (r < target)
            --insertAt;
    for (int i = 0; i < taken.size(); ++i) {
        if (rows.at(i) == current)
            newCurrent = insertAt + i;
        model->insertRow(insertAt + i, taken.at(i));
    }

    if (newCurrent >= 0)
        listView->setCurrentIndex(model->index(newCurrent, 0));
}


void TabHandlerSendCommand::clearResponse()
{
    textEditResponse->clear();
    lineEditSw->clear();
    lineEditSwText->clear();
}


void TabHandlerSendCommand::showResponse(const QByteArray &response)
{
    textEditResponse->setPlainText(Utility::byteArrayToHexString(response));
    if (response.size() < 2) {
        lineEditSw->clear();
        lineEditSwText->clear();
        return;
    }
    const quint8 sw1 = static_cast<quint8>(response[response.size() - 2]);
    const quint8 sw2 = static_cast<quint8>(response[response.size() - 1]);
    lineEditSw->setText(Utility::byteArrayToHexString(response.right(2)));
    lineEditSwText->setText(Utility::statusWordText(sw1, sw2));
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
        } else if (event->type() == QEvent::Drop && static_cast<QDropEvent *>(event)->mimeData()->hasUrls()) {
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
