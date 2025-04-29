#ifndef TABHANDLERRUNSCRIPT_H
#define TABHANDLERRUNSCRIPT_H

#include <QObject>
#include <QWidget>
#include <QStackedWidget>
#include <QPushButton>

#include "global.h"
#include "scmapplication.h"


class TabHandlerRunScript : public QWidget
{
    Q_OBJECT

public:
    explicit TabHandlerRunScript(QWidget *tabWidget, QWidget *parent = nullptr);
    ~TabHandlerRunScript();

private:
    void replaceCurrentView(QWidget *view);
    void buttonActionAdd();

private:
    ScmApplication *app;

    QStackedWidget *stackedWidget;
    QPushButton *buttonAdd;

signals:
    void logBytes(LogType type, const QByteArray& hexBytes);
    void logString(LogType type, const QString& hexString);
    void update();
};

#endif
