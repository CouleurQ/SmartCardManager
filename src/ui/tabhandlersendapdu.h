#ifndef TABHANDLERSENDAPDU_H
#define TABHANDLERSENDAPDU_H

#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QListView>
#include "hexedit.h"

#include "global.h"
#include "scmapplication.h"


class TabHandlerSendApdu : public QWidget
{
    Q_OBJECT

public:
    explicit TabHandlerSendApdu(QWidget *tabWidget, QWidget *parent = nullptr);
    ~TabHandlerSendApdu();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

private:
    void onItemSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void onItemSelected(const QModelIndex &current, const QModelIndex &previous);
    void onItemClicked(const QModelIndex &index);
    void onItemDoubleClicked(const QModelIndex &index);
    void updateCommandName(const QString &newText);
    void updateCommandApdu();
    void showContextMenu(const QPoint &pos);

    void buttonActionLoad();
    void _buttonActionLoad(const QString &filePath);
    void buttonActionSave();
    void buttonActionSend();
    void buttonActionAdd();
    void _buttonActionAdd(const QString &cmdName, const QByteArray &cmdData);
    void buttonActionClear();
    void buttonActionDelete();
    void buttonActionDuplicate();
    void buttonActionUp();
    void buttonActionDown();

private:
    ScmApplication *app;

    QPushButton *buttonLoad;
    QPushButton *buttonSave;
    QPushButton *buttonSend;
    QLineEdit *lineEditFilename;
    QListView *listView;
    QLineEdit *lineEditCommandName;
    HexEdit *hexEditCommandApdu;

signals:
    void logBytes(LogType type, const QByteArray& hexBytes);
    void logString(LogType type, const QString& hexString);
    void update();
};

#endif
