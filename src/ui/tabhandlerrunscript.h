#ifndef TABHANDLERRUNSCRIPT_H
#define TABHANDLERRUNSCRIPT_H

#include <QObject>
#include <QWidget>
#include <QStackedWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QStandardItem>

#include "global.h"
#include "commands/commandview.h"
#include "commands/command.h"
#include "commands/runscriptmodel.h"
#include "commands/scriptrunner.h"

class CardSession;
class ListReorderHelper;


class TabHandlerRunScript : public QObject
{
    Q_OBJECT

public:
    TabHandlerRunScript(QWidget *tabWidget, CardSession *session, QObject *parent = nullptr);
    ~TabHandlerRunScript();

private:
    void replaceCurrentView(CommandView *view);
    void showContextMenu(const QPoint &pos);
    void showItem(const QModelIndex &index);
    void onItemSelected(const QModelIndex &current, const QModelIndex &previous);
    void buttonActionAdd();
    void buttonActionLoad();
    void _buttonActionLoad(const QString &filePath);
    void buttonActionSave();
    void buttonActionClear();
    void buttonActionDelete();
    void buttonActionDuplicate();
    void buttonActionUp();
    void buttonActionDown();
    void buttonActionRun();
    void buttonActionStep();
    void buttonActionStop();
    void startRun(const QList<QSharedPointer<Command>> &commands, int firstRow);
    void onCommandStarted(int row);
    void onCommandFinished(int row, bool ok);
    void onRunFinished(bool success, int executed);
    void updateRunState();

private:
    QWidget *tabWidget;
    CardSession *session;
    ScriptRunner *runner;
    int stepTarget = -1;

    RunScriptModel *model;
    QListView *listView;
    ListReorderHelper *reorder;
    QStackedWidget *stackedWidget;
    QPushButton *buttonAdd;
    QPushButton *buttonLoad;
    QPushButton *buttonSave;
    QPushButton *buttonRun;
    QPushButton *buttonStep;
    QPushButton *buttonStop;
    QCheckBox *checkStopOnError;
    QLineEdit *lineEditFilename;

private slots:
    void receiveUpdateCommand(QSharedPointer<Command> obj);

signals:
    void logBytes(LogType type, const QByteArray& hexBytes);
    void logString(LogType type, const QString& hexString);
    void update();
};

#endif
