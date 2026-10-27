#include "tabhandlerrunscript.h"
#include "card/cardsession.h"
#include "io/scriptfile.h"
#include "commands/commandfactory.h"
#include "commands/commandview.h"
#include "commands/commandviewsendbytes.h"
#include "commands/commandviewapdu.h"
#include "commands/commandviewterminal.h"
#include "commands/commandsendbytes.h"
#include "commands/commandapdu.h"
#include "commands/commandterminal.h"
#include "commands/runscriptmodel.h"

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
#include <QItemSelectionModel>
#include "listreorderhelper.h"
#include <algorithm>



namespace {
CommandView *createView(const QSharedPointer<Command> &command)
{
    const QString type = command->typeName();
    if (type == "SendBytes")
        return new CommandViewSendBytes(qSharedPointerCast<CommandSendBytes>(command));
    if (type == "SendAPDU")
        return new CommandViewApdu(qSharedPointerCast<CommandApdu>(command));
    if (type == "TerminalCmd")
        return new CommandViewTerminal(qSharedPointerCast<CommandTerminal>(command));
    return nullptr;
}
}


TabHandlerRunScript::TabHandlerRunScript(QWidget *tabWidget, CardSession *session, QObject *parent)
    : QObject{parent}
    , tabWidget(tabWidget)
    , session(session)
{
    Q_ASSERT(session != nullptr);

    // load the widgets
    buttonLoad = tabWidget->findChild<QPushButton *>("runScriptButtonLoad");
    Q_ASSERT(buttonLoad != nullptr);
    buttonSave = tabWidget->findChild<QPushButton *>("runScriptButtonSave");
    Q_ASSERT(buttonSave != nullptr);
    buttonRun = tabWidget->findChild<QPushButton *>("runScriptButtonRun");
    Q_ASSERT(buttonRun != nullptr);
    buttonStep = tabWidget->findChild<QPushButton *>("runScriptButtonStep");
    Q_ASSERT(buttonStep != nullptr);
    checkStopOnError = tabWidget->findChild<QCheckBox *>("runScriptCheckStopOnError");
    Q_ASSERT(checkStopOnError != nullptr);
    buttonStop = tabWidget->findChild<QPushButton *>("runScriptButtonStop");
    Q_ASSERT(buttonStop != nullptr);
    stackedWidget = tabWidget->findChild<QStackedWidget *>("runScriptStackedWidget");
    Q_ASSERT(stackedWidget != nullptr);
    buttonAdd = tabWidget->findChild<QPushButton *>("runScriptButtonAdd");
    Q_ASSERT(buttonAdd != nullptr);
    listView = tabWidget->findChild<QListView *>("runScriptListView");
    Q_ASSERT(listView != nullptr);
    lineEditFilename = tabWidget->findChild<QLineEdit *>("runScriptLineEditFilename");
    Q_ASSERT(lineEditFilename != nullptr);

    // Add actions to the menu
    QMenu *menu = new QMenu(tabWidget);
    QAction *action1 = menu->addAction("Send Bytes");
    action1->setData(CommandSendBytes().typeName());
    QAction *action2 = menu->addAction("Send APDU");
    action2->setData(CommandApdu().typeName());
    QAction *action3 = menu->addAction("Terminal command");
    action3->setData(CommandTerminal().typeName());
    buttonAdd->setMenu(menu);

    // add the model to the listview
    model = new RunScriptModel(this);
    listView->setModel(model);
    listView->setContextMenuPolicy(Qt::CustomContextMenu);
    listView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    reorder = new ListReorderHelper(listView, [this](const QList<int> &rows, int target) {
        if (!runner->isRunning())
            model->moveToRow(rows, target);
    });

    runner = new ScriptRunner(session, this);

    // Connect the actions to slots
    connect(buttonRun, &QPushButton::clicked, this, &TabHandlerRunScript::buttonActionRun);
    connect(buttonStep, &QPushButton::clicked, this, &TabHandlerRunScript::buttonActionStep);
    connect(buttonStop, &QPushButton::clicked, this, &TabHandlerRunScript::buttonActionStop);
    connect(runner, &ScriptRunner::commandStarted, this, &TabHandlerRunScript::onCommandStarted);
    connect(runner, &ScriptRunner::commandFinished, this, &TabHandlerRunScript::onCommandFinished);
    connect(runner, &ScriptRunner::finished, this, &TabHandlerRunScript::onRunFinished);
    connect(runner, &ScriptRunner::logBytes, this, &TabHandlerRunScript::logBytes);
    connect(runner, &ScriptRunner::logString, this, &TabHandlerRunScript::logString);
    connect(model, &QAbstractItemModel::rowsInserted, this, &TabHandlerRunScript::updateRunState);
    connect(model, &QAbstractItemModel::rowsRemoved, this, &TabHandlerRunScript::updateRunState);
    connect(model, &QAbstractItemModel::modelReset, this, &TabHandlerRunScript::updateRunState);
    connect(buttonLoad, &QPushButton::clicked, this, &TabHandlerRunScript::buttonActionLoad);
    connect(buttonSave, &QPushButton::clicked, this, &TabHandlerRunScript::buttonActionSave);
    connect(action1, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(action2, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(action3, &QAction::triggered, this, &TabHandlerRunScript::buttonActionAdd);
    connect(listView, &QListView::customContextMenuRequested, this, &TabHandlerRunScript::showContextMenu);
    connect(listView->selectionModel(), &QItemSelectionModel::currentChanged, this, &TabHandlerRunScript::onItemSelected);

    updateRunState();
}


TabHandlerRunScript::~TabHandlerRunScript()
{
    runner->stop();
}


void TabHandlerRunScript::showContextMenu(const QPoint &pos)
{
    if (runner->isRunning())
        return;

    QModelIndex index = listView->indexAt(pos);

    QMenu menu(tabWidget);
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


void TabHandlerRunScript::showItem(const QModelIndex &index)
{
    QSharedPointer<Command> item = model->itemFromIndex(index);
    if (!item)
        return;

    replaceCurrentView(createView(item));
}


void TabHandlerRunScript::onItemSelected(const QModelIndex &current, const QModelIndex &previous)
{
    (void) previous;

    if (current.isValid())
        showItem(current);
    else
        replaceCurrentView(nullptr);
}


void TabHandlerRunScript::replaceCurrentView(CommandView *view) {
    while (stackedWidget->count() > 0) {
        QWidget *currentView = stackedWidget->widget(0);
        stackedWidget->removeWidget(currentView);
        currentView->disconnect(this);
        currentView->deleteLater();
    }

    if (view != nullptr) {
        view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        stackedWidget->addWidget(view);
        stackedWidget->setCurrentWidget(view);

        connect(view, &CommandView::notifyUpdateCommand, this, &TabHandlerRunScript::receiveUpdateCommand);
    }
}


void TabHandlerRunScript::buttonActionDelete()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid())
        model->removeCommand(currentIndex.row());
}


void TabHandlerRunScript::buttonActionDuplicate()
{
    QModelIndex currentIndex = listView->selectionModel()->currentIndex();

    if (currentIndex.isValid()) {
        QSharedPointer<Command> newItem = model->itemFromIndex(currentIndex)->clone();
        newItem->commandName.append("_copy");
        model->addCommand(newItem);

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
            model->insertCommands(row - 1, model->takeCommands(row));
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
            model->insertCommands(row + 1, model->takeCommands(row));
            listView->setCurrentIndex(model->index(row + 1, 0));
        }
    }
}


void TabHandlerRunScript::buttonActionAdd()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action == NULL)
        return;

    QSharedPointer<Command> command = CommandFactory::create(action->data().toString());
    if (!command)
        return;
    model->addCommand(command);

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
    replaceCurrentView(nullptr);
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
    QList<QSharedPointer<Command>> commands;
    QString error;
    if (!ScriptFile::load(filePath, &commands, &error)) {
        emit logString(LogType::Error, error);
        return;
    }

    // file seems valid - replace the old entries
    buttonActionClear();
    for (const auto &command : commands)
        model->addCommand(command);

    lineEditFilename->setText(filePath);

    // focus the first item
    if (model->rowCount() == 0)
        return;
    QModelIndex index = model->index(0, 0);
    listView->setCurrentIndex(index);
    listView->scrollTo(index);
    listView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
}


void TabHandlerRunScript::buttonActionSave()
{
    QString filePath = QFileDialog::getSaveFileName(tabWidget, "Save File", "", "JSON Files (*.json);;All Files (*)");

    if (filePath.isEmpty())
        return;

    if (!filePath.endsWith(".json", Qt::CaseInsensitive)) {
        filePath.append(".json");
    }

    QString error;
    if (!ScriptFile::save(filePath, model->commands(), &error)) {
        emit logString(LogType::Error, error);
        return;
    }

    lineEditFilename->setText(filePath);
    emit logString(LogType::Info, QString("%1: %2").arg("Script saved", filePath));
}


void TabHandlerRunScript::buttonActionRun()
{
    if (model->rowCount() == 0)
        return;

    model->clearStatus();
    stepTarget = -1;

    // two or more selected rows: run only those, otherwise the whole script
    QList<int> rows;
    for (const QModelIndex &index : listView->selectionModel()->selectedRows())
        rows.append(index.row());
    std::sort(rows.begin(), rows.end());
    if (rows.size() < 2 || rows.size() == model->rowCount()) {
        startRun(model->commands(), 0);
        return;
    }

    QList<QSharedPointer<Command>> commands;
    for (int row : rows)
        commands.append(model->item(row));
    if (runner->start(commands, rows, checkStopOnError->isChecked()))
        updateRunState();
}


void TabHandlerRunScript::buttonActionStep()
{
    const int row = listView->currentIndex().isValid() ? listView->currentIndex().row() : 0;
    const QSharedPointer<Command> command = model->item(row);
    if (!command)
        return;

    model->setStatus(row, RunScriptModel::Status::None);
    stepTarget = row;
    startRun({command}, row);
}


void TabHandlerRunScript::buttonActionStop()
{
    runner->stop();
}


void TabHandlerRunScript::startRun(const QList<QSharedPointer<Command>> &commands, int firstRow)
{
    if (runner->start(commands, firstRow, checkStopOnError->isChecked()))
        updateRunState();
}


void TabHandlerRunScript::onCommandStarted(int row)
{
    model->setStatus(row, RunScriptModel::Status::Running);
    listView->scrollTo(model->index(row, 0));
}


void TabHandlerRunScript::onCommandFinished(int row, bool ok)
{
    model->setStatus(row, ok ? RunScriptModel::Status::Passed : RunScriptModel::Status::Failed);
}


void TabHandlerRunScript::onRunFinished(bool success, int executed)
{
    Q_UNUSED(executed)
    updateRunState();

    // stepping moves on to the next command
    if (stepTarget >= 0 && success && stepTarget + 1 < model->rowCount())
        listView->setCurrentIndex(model->index(stepTarget + 1, 0));
    stepTarget = -1;

    emit update();
}


void TabHandlerRunScript::updateRunState()
{
    const bool running = runner->isRunning();
    const bool hasCommands = model->rowCount() > 0;

    listView->setEnabled(!running);
    stackedWidget->setEnabled(!running);
    buttonAdd->setEnabled(!running);
    buttonLoad->setEnabled(!running);
    buttonSave->setEnabled(!running);
    buttonRun->setEnabled(!running && hasCommands);
    buttonStep->setEnabled(!running && hasCommands);
    buttonStop->setEnabled(running);
    checkStopOnError->setEnabled(!running);
}
