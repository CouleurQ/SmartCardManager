#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"

#include <QToolBar>
#include <QTabWidget>
#include <QLabel>
#include <QDockWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QApplication>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // menubar
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionLogWindow, &QAction::triggered, this, &MainWindow::actionLogWindow);
    connect(ui->dockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::actionLogWindowState);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::actionAbout);

    ui->logTextEdit->append("Application started.");
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::actionLogWindow()
{
    if(ui->dockWidget->isVisible())
        ui->dockWidget->hide();
    else
        ui->dockWidget->show();
}

void MainWindow::actionLogWindowState() {
    ui->actionLogWindow->setChecked(ui->dockWidget->isVisible());
}


void MainWindow::actionAbout()
{
    AboutDialog *aboutDialog = new AboutDialog(this);
    aboutDialog->exec();
    delete aboutDialog;
}
