#ifndef TABHANDLERRUNSCRIPT_H
#define TABHANDLERRUNSCRIPT_H

#include <QObject>
#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QListWidget>

#include "global.h"
#include "scmapplication.h"
#include "commands/view.h"
#include "commands/command.h"


class TabHandlerRunScript : public QWidget
{
    Q_OBJECT

public:
    explicit TabHandlerRunScript(QWidget *tabWidget, QWidget *parent = nullptr);
    ~TabHandlerRunScript();

private:
    void replaceCurrentView(View *view);
    void buttonActionAdd();

private:
    ScmApplication *app;

    QListView *listView;
    QStackedWidget *stackedWidget;
    QPushButton *buttonAdd;

private slots:
    void receiveUpdateCommand(QSharedPointer<Command> obj);

signals:
    void logBytes(LogType type, const QByteArray& hexBytes);
    void logString(LogType type, const QString& hexString);
    void update();
};

#endif
