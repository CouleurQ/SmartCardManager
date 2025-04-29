#include "viewapdu.h"
#include "ui_viewapdu.h"
#include "commandapdu.h"
#include "utility.h"

#include <QTimer>


ViewApdu::ViewApdu(QWidget *parent) :
    View(parent),
    ui(new Ui::ViewApdu)
{
    ui->setupUi(this);

    ui->lineEditCommandName->setText("Send APDU");

    connect(ui->lineEditCommandName, &QLineEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->lineEditCLA, &QLineEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->lineEditINS, &QLineEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->lineEditP1, &QLineEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->lineEditP2, &QLineEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->hexEditData, &HexEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->lineEditLe, &QLineEdit::textChanged, this, &ViewApdu::sendUpdateCommand);
    connect(ui->hexEditExpectedResponse, &HexEdit::textChanged, this, &ViewApdu::sendUpdateCommand);

    // expected response is single line
    ui->hexEditExpectedResponse->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditExpectedResponse->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // set the focus (delayed)
    QTimer::singleShot(0, this, [this]() {
        ui->lineEditCommandName->setFocus();
    });
}


ViewApdu::~ViewApdu()
{
    delete ui;
}


void ViewApdu::sendUpdateCommand()
{
    QSharedPointer<CommandApdu> obj = QSharedPointer<CommandApdu>::create();

    obj->commandName = ui->lineEditCommandName->text();
    obj->commandData = Utility::hexStringToByteArray(ui->hexEditData->toPlainText());
    obj->expectedResponse = Utility::hexStringToByteArray(ui->hexEditExpectedResponse->toPlainText());

    emit notifyUpdateCommand(obj);

    QByteArray lc = Utility::hexStringToByteArray(ui->hexEditData->toPlainText());
    ui->lineEditLc->setText(QString("%1").arg(QString::number(lc.length(), 16).toUpper()));
}
