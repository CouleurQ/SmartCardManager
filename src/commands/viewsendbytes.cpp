#include "viewsendbytes.h"
#include "ui_viewsendbytes.h"
#include "commandsendbytes.h"
#include "utility.h"

#include <QTimer>


ViewSendBytes::ViewSendBytes(QWidget *parent) :
    View(parent),
    ui(new Ui::ViewSendBytes)
{
    ui->setupUi(this);

    ui->lineEditCommandName->setText("Send bytes");

    connect(ui->lineEditCommandName, &QLineEdit::textChanged, this, &ViewSendBytes::sendUpdateCommand);
    connect(ui->hexEditCommandApdu, &HexEdit::textChanged, this, &ViewSendBytes::sendUpdateCommand);
    connect(ui->hexEditExpectedResponse, &HexEdit::textChanged, this, &ViewSendBytes::sendUpdateCommand);

    // expected response is single line
    ui->hexEditExpectedResponse->setLineWrapMode(QTextEdit::NoWrap);
    ui->hexEditExpectedResponse->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // set the focus (delayed)
    QTimer::singleShot(0, this, [this]() {
        ui->lineEditCommandName->setFocus();
    });
}


ViewSendBytes::~ViewSendBytes()
{
    delete ui;
}


void ViewSendBytes::sendUpdateCommand()
{
    QSharedPointer<CommandSendBytes> obj = QSharedPointer<CommandSendBytes>::create();

    obj->commandName = ui->lineEditCommandName->text();
    obj->commandData = Utility::hexStringToByteArray(ui->hexEditCommandApdu->toPlainText());
    obj->expectedResponse = Utility::hexStringToByteArray(ui->hexEditExpectedResponse->toPlainText());

    emit notifyUpdateCommand(obj);
}
