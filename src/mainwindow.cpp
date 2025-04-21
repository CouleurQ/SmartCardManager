#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "aboutdialog.h"
#include "logview.h"

#include <QToolBar>
#include <QTabWidget>
#include <QLabel>
#include <QDockWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QApplication>
#include <QTimer>
#include <QMessageBox>
#include <QTreeView>
#include <QStandardItem>
#include <QIcon>

#include <pcsclite.h>
#include <winscard.h>


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

    logView = new LogView(ui->logView, this);

    QTimer::singleShot(0, this, &MainWindow::onViewCreated);
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::onViewCreated()
{
    ui->comboBox->clear();

    SCARDCONTEXT hContext;
    LONG ret = SCardEstablishContext(SCARD_SCOPE_SYSTEM, nullptr, nullptr, &hContext);
    if (ret != SCARD_S_SUCCESS) {
        logView->addLogEntry(LogView::Error, "Failed to establish context - PCSC service not running?");
        return;
    }

    // Get the list of readers
    char readers[256];
    DWORD readersLen = sizeof(readers);
    ret = SCardListReaders(hContext, nullptr, readers, &readersLen);
    if (ret != SCARD_S_SUCCESS) {
        SCardReleaseContext(hContext);
        return;
    }

    // Add readers to the combo box
    char *reader = readers;
    while (*reader != '\0') {
        ui->comboBox->addItem(QString::fromUtf8(reader));
        reader += strlen(reader) + 1;
    }

    // Clean up
    //SCardFreeMemory(hContext, readers);
    SCardReleaseContext(hContext);
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
