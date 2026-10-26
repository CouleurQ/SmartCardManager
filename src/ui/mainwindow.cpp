#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scmapplication.h"
#include "aboutdialog.h"
#include "logview.h"
#include "pcscwrapper.h"
#include "tabhandlersendcommand.h"

#include <QToolBar>
#include <QTabWidget>
#include <QLabel>
#include <QDockWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QApplication>
#include <QTimer>
#include <QTreeView>
#include <QStandardItem>
#include <QIcon>
#include <QDesktopServices>
#include <QSvgWidget>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    app = qApp ? qobject_cast<ScmApplication*>(qApp) : nullptr;
    Q_ASSERT_X(app, "MainWindow", "QApplication must be a ScmApplication");

    // Toolbar
    cardReaderCombo = new QComboBox(this);
    cardReaderCombo->setMinimumWidth(350);
    QToolBar *toolbar = ui->toolBar;
    toolbar->insertWidget(ui->actionRefresh, cardReaderCombo);

    // Statusbar
    statusLine = new QLineEdit("Ready");
    statusLine->setReadOnly(true);
    statusLine->setFocusPolicy(Qt::NoFocus);
    statusLine->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    statusLine->setStyleSheet("background: transparent; border: none; color: black;");
    statusLine->setFrame(false);
    statusLine->setMinimumWidth(150);
    ui->statusbar->addWidget(statusLine);

    QSvgWidget* svgIcon = new QSvgWidget(":/res/icons/status_busy.svg");
    svgIcon->setFixedSize(12, 12);
    ui->statusbar->addPermanentWidget(svgIcon);

    QWidget* spacer = new QWidget();
    spacer->setFixedSize(1, 1);
    ui->statusbar->addPermanentWidget(spacer);

    connectionIcon = new QSvgWidget(":/res/icons/action_disconnect.svg");
    connectionIcon->setFixedSize(16, 16);
    ui->statusbar->addPermanentWidget(connectionIcon);

    // Splitter
    ui->splitterTab->setSizes({250, 550});
    ui->runScriptSplitterTab->setSizes({250, 550});

    // Logview
    logView = new LogView(ui->logView, this);

    // TabViews
    tabSendCommand = new TabHandlerSendCommand(ui->tabSendCommand, this);
    tabRunScript = new TabHandlerRunScript(ui->tabRunScript, this);

    // Menubar
    connect(ui->actionExit, &QAction::triggered, this, &QMainWindow::close);
    connect(ui->actionLogWindow, &QAction::triggered, this, &MainWindow::actionLogWindow);
    connect(ui->dockWidget, &QDockWidget::visibilityChanged, this, &MainWindow::actionLogWindowState);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::actionAbout);
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::actionHelp);
    connect(ui->actionRefresh, &QAction::triggered, this, &MainWindow::actionRefresh);
    // Toolbar
    connect(ui->actionConnect, &QAction::triggered, this, &MainWindow::actionConnect);
    connect(ui->actionDisconnect, &QAction::triggered, this, &MainWindow::actionDisconnect);
    connect(ui->actionReset, &QAction::triggered, this, &MainWindow::actionReset);
    connect(cardReaderCombo, &QComboBox::currentIndexChanged, this, &MainWindow::actionReaderChanged);


    // signal slots
    connect(app->pcsc, &PcscWrapper::log, this, &MainWindow::logString);
    connect(tabSendCommand, &TabHandlerSendCommand::logBytes, this, &MainWindow::logBytes);
    connect(tabSendCommand, &TabHandlerSendCommand::logString, this, &MainWindow::logString);
    connect(tabSendCommand, &TabHandlerSendCommand::update, this, &MainWindow::updateUiState);
    connect(tabRunScript, &TabHandlerRunScript::logBytes, this, &MainWindow::logBytes);
    connect(tabRunScript, &TabHandlerRunScript::logString, this, &MainWindow::logString);
    connect(tabRunScript, &TabHandlerRunScript::update, this, &MainWindow::updateUiState);

    // asynchronous call to update the combobox in the toolbar
    QTimer::singleShot(0, this, &MainWindow::actionRefresh);

    updateUiState();
}


MainWindow::~MainWindow()
{
    // the dock emits visibilityChanged while the window is being torn down
    ui->dockWidget->disconnect(this);
    delete ui;
}


void MainWindow::logBytes(const LogType type, const QByteArray& data)
{
    logView->addLogEntry(type, data);
}


void MainWindow::logString(const LogType type, const QString& data)
{
    logView->addLogEntry(type, data);
}


void MainWindow::updateUiState()
{
    bool connected = app->pcsc->isConnected(app->cardReader);

    // Toolbar states
    ui->actionConnect->setEnabled(!connected);
    ui->actionDisconnect->setEnabled(connected);
    ui->actionReset->setEnabled(connected);

    // Statusbar
    connectionIcon->load(QString(connected ? ":/res/icons/action_connect.svg" : ":/res/icons/action_disconnect.svg"));
    if (connected)
        statusLine->setText(QString("Connected: %1").arg(app->cardReader));
    else if (app->cardReader.isEmpty())
        statusLine->setText("No reader selected");
    else
        statusLine->setText(QString("Ready: %1").arg(app->cardReader));
}


void MainWindow::actionConnect()
{
    logView->addLogEntry(LogType::Terminal, QString("%1: %2").arg("Connect card in reader", app->cardReader));
    app->pcsc->connect(app->cardReader);
    updateUiState();
}


void MainWindow::actionDisconnect()
{
    logView->addLogEntry(LogType::Terminal, QString("%1: %2").arg("Disconnect card in reader", app->cardReader));
    app->pcsc->disconnect(app->cardReader);
    updateUiState();
}


void MainWindow::actionReset()
{
    logView->addLogEntry(LogType::Terminal, QString("%1: %2").arg("Reset card in reader", app->cardReader));
    app->pcsc->reset(app->cardReader);
    updateUiState();
}


void MainWindow::actionRefresh()
{
    const QString previousReader = app->cardReader;

    cardReaderCombo->blockSignals(true);
    cardReaderCombo->clear();
    cardReaderCombo->addItems(app->pcsc->getReaders());
    const int previousIndex = cardReaderCombo->findText(previousReader);
    cardReaderCombo->setCurrentIndex(previousIndex >= 0 ? previousIndex : (cardReaderCombo->count() > 0 ? 0 : -1));
    cardReaderCombo->blockSignals(false);

    // a reader that went away invalidates the connection and the selection
    if (cardReaderCombo->currentText() != previousReader)
        actionReaderChanged();
    else
        updateUiState();
}


void MainWindow::actionReaderChanged()
{
    if (app->pcsc->isConnected(app->cardReader))
        app->pcsc->disconnect(app->cardReader);

    app->cardReader = cardReaderCombo->currentText();
    updateUiState();
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
    AboutDialog aboutDialog(this);
    aboutDialog.exec();
}


void MainWindow::actionHelp()
{
    QUrl qUrl("https://github.com/CouleurQ/SmartCardManager/wiki");
    if (qUrl.isValid()) {
        QDesktopServices::openUrl(qUrl);
    }
}
