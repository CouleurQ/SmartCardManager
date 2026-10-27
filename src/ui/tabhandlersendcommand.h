#ifndef TABHANDLERSENDCOMMAND_H
#define TABHANDLERSENDCOMMAND_H

#include <QObject>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QListView>
#include <QTextEdit>
#include "hexedit.h"

#include "global.h"
#include "card/cardsession.h"


class ListReorderHelper;

class TabHandlerSendCommand : public QObject
{
    Q_OBJECT

public:
    TabHandlerSendCommand(QWidget *tabWidget, CardSession *session, QObject *parent = nullptr);
    ~TabHandlerSendCommand();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
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
    void onResponseReceived(const QByteArray &command, const QByteArray &response);
    void buttonActionAdd();
    void _buttonActionAdd(const QString &cmdName, const QByteArray &cmdData);
    void buttonActionClear();
    void buttonActionDelete();
    void buttonActionDuplicate();
    void buttonActionUp();
    void buttonActionDown();

private:
    CardSession *session;
    QWidget *tabWidget;

    QPushButton *buttonLoad;
    QPushButton *buttonSave;
    QPushButton *buttonAdd;
    QPushButton *buttonSend;
    QLineEdit *lineEditFilename;
    QListView *listView;
    QLineEdit *lineEditCommandName;
    HexEdit *hexEditCommandApdu;
    QLineEdit *lineEditSw;
    QLineEdit *lineEditSwText;
    QTextEdit *textEditResponse;

    void showResponse(const QByteArray &response);
    void clearResponse();
    void moveItems(const QList<int> &rows, int target);
    ListReorderHelper *reorder;
    QPersistentModelIndex sentIndex;
    bool sentPending = false;

signals:
    void logBytes(LogType type, const QByteArray& hexBytes);
    void logString(LogType type, const QString& hexString);
    void update();
};

#endif
