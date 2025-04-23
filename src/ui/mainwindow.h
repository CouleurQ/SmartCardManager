#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QComboBox>

#include "scmapplication.h"
#include "logview.h"
#include "tabhandlersendapdu.h"


QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE



class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    ScmApplication *app;
    Ui::MainWindow *ui;

    LogView *logView;
    QComboBox *cardReaderCombo;
    TabHandlerSendApdu *tabSendApdu;

private slots:
    void logBytes(const LogType type, const QByteArray& data);
    void logString(const LogType type, const QString& data);

private:
    void updateUiState();

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
