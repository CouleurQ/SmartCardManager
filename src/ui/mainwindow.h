#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QComboBox>
#include <QLineEdit>
#include <QSvgWidget>

#include "card/cardsession.h"
#include "atrview.h"
#include "javacardview.h"
#include "logview.h"
#include "tabhandlersendcommand.h"
#include "tabhandlerrunscript.h"


QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(CardSession *session, QWidget *parent = nullptr);
    ~MainWindow();

private:
    CardSession *session;
    Ui::MainWindow *ui;

    LogView *logView;
    QComboBox *cardReaderCombo;
    QLabel *statusLine;
    QLabel *channelStatus;
    QLabel *secureChannelStatus;
    QSvgWidget *connectionIcon;
    AtrView *atrView;
    JavaCardView *javaCardView;
    TabHandlerSendCommand *tabSendCommand;
    TabHandlerRunScript *tabRunScript;
    bool busy = false;      // a connect/disconnect/reset is running in the background

private slots:
    void logBytes(const LogType type, const QByteArray& data);
    void logString(const LogType type, const QString& data);

private:
    void updateUiState();
    void onSessionStateChanged();
    void onReadersListed(const QStringList &readers);

    void actionConnect();
    void actionDisconnect();
    void actionReset();
    void actionReaderChanged();

    void actionRefresh();
    void actionLogWindow();
    void actionLogWindowState();
    void actionHelp();
    void actionAbout();
};
#endif
