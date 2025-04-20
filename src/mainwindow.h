#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


QT_BEGIN_NAMESPACE
namespace Ui {
    class MainWindow;
}
QT_END_NAMESPACE


class QTextEdit;
class QTabWidget;


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QTabWidget *tabWidget;
    QTextEdit *logTextEdit;

private:
    void actionLogWindow();
    void actionLogWindowState();
    void actionHelp();
    void actionAbout();
};
#endif
