#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QComboBox>

#include "logview.h"
#include "pcscwrapper.h"


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
    Ui::MainWindow *ui;

    PcscWrapper *pcsc;
    LogView *logView;
    QComboBox *cardReaderCombo;

private slots:
    void onPcscLog(const LogType type, const QString& hexData);

private:
    void updateUiState();

    void sendApdu();

    void actionConnect();
    void actionDisconnect();
    void actionReset();

    void actionRefresh();
    void actionLogWindow();
    void actionLogWindowState();
    void actionHelp();
    void actionAbout();
};
#endif
