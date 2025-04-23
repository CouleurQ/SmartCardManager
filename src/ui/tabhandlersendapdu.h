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

private:
    void sendApduLoad();
    void sendApduSave();
    void sendApduSend();
    void onItemSelectionChanged(const QModelIndex &current, const QModelIndex &previous);
    void onItemSelected(const QItemSelection &selected, const QItemSelection &deselected);
    void onItemClicked(const QModelIndex &index);
    void onItemDoubleClicked(const QModelIndex &index);
    void updateCommandName(const QString &newText);
    void updateCommandApdu(const QString &newText);

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
