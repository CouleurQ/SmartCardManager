#ifndef TABHANDLERRUNSCRIPT_H
#define TABHANDLERRUNSCRIPT_H

#include <QObject>
#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QStandardItem>

#include "global.h"
#include "scmapplication.h"
#include "commands/view.h"
#include "commands/command.h"
#include "commands/model.h"


class TabHandlerRunScript : public QWidget
{
    Q_OBJECT

public:
    explicit TabHandlerRunScript(QWidget *tabWidget, QWidget *parent = nullptr);
    ~TabHandlerRunScript();

private:
    void replaceCurrentView(View *view);
    void showContextMenu(const QPoint &pos);
    void onItemClicked(const QModelIndex &index);
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

private:
    ScmApplication *app;

    RunScriptModel *model;
    QListView *listView;
    QStackedWidget *stackedWidget;
    QPushButton *buttonAdd;
    QPushButton *buttonLoad;
    QPushButton *buttonSave;
    QLineEdit *lineEditFilename;

private slots:
    void receiveUpdateCommand(QSharedPointer<Command> obj);

signals:
    void logBytes(LogType type, const QByteArray& hexBytes);
    void logString(LogType type, const QString& hexString);
    void update();
};

#endif
